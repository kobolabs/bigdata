/*
 *  Copyright (C) 2014 Kobo Inc.
 *  All rights reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS
 *  IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 *  express or implied.  See the License for the specific language
 *  governing permissions and limitations under the License.
 */

//read_json uses boost::spirit, which is not thread-safe by default
#define BOOST_SPIRIT_THREADSAFE

#include "gibbs.hpp"

#include <random>
#include <sstream>
#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/atomic.hpp>
#include <boost/foreach.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/thread/thread.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/lexical_cast.hpp>

boost::atomic_int thread_index_(0);
// Lockfree Queue to process raw word-count strings
boost::lockfree::queue<index_t> queue(1000000);

// Random number generator
std::default_random_engine generator;
std::uniform_real_distribution<double> distribution(0.0, 1.0);

// in C+11, can use std::discrete_distribution
template<typename Double>
size_t multinomial(const std::vector<Double>& prb) {
    assert(prb.size() > 0);
    if (prb.size() == 1) 
        return 0;

    Double sum(0);
    for(size_t i = 0; i < prb.size(); ++i) {
        assert(prb[i] >= 0); // Each entry must be non-negative
        sum += prb[i];
    }
    assert(sum > 0); // Normalizer must be positive
    // Draw a random number
    const Double rnd(distribution(generator));
    size_t ind = 0;
    for(Double cumsum(prb[ind]/sum); 
            rnd > cumsum && (ind+1) < prb.size(); 
            cumsum += (prb[++ind]/sum));
    return ind;
}

Gibbs::Gibbs(std::string outputfile, count_t ntopics, double alpha, double beta, count_t burnin): 
    output_file_dir_(outputfile), ntopics_(ntopics), ALPHA_(alpha), BETA_(beta), burnin_(burnin) {
    ntokens_ = 0;
    global_topic_counts_.resize(ntopics_);
}

Gibbs::~Gibbs() {}

void Gibbs::import_token_topic_count(std::string token_topic_count_file) {
    std::ifstream token_topic_count_stream(token_topic_count_file);
    if(!token_topic_count_stream.is_open()) {
        std::cout << "Unable to open file: " << token_topic_count_file << std::endl;
        exit(1);
    }
    // Line format: integer_token_hash \t [0, 20, 72, 0, ...]
    std::string line;
    std::cout << "Importing the topic distributions of all tokens..." << std::endl;
    while (getline(token_topic_count_stream, line)) {
        std::vector<std::string> strs;
        //strs[0] is the integer token_hash, strs[1] is a str of json array
        boost::split(strs, line, boost::is_any_of("\t"));
        assert(strs.size() == 2);

        token_t token_hash = boost::lexical_cast<token_t>(strs[0]); 
        std::stringstream ss(strs[1]);
        boost::property_tree::ptree pt;
        boost::property_tree::read_json(ss, pt);

        token_topic_count_t token_topic_vec;
        index_t topic_index = 0;
        BOOST_FOREACH(boost::property_tree::ptree::value_type &v, pt) {
            assert(v.first.empty()); // array elements have no names
            count_t c = boost::lexical_cast<count_t>(v.second.data());
            token_topic_vec.push_back(c);
            global_topic_counts_[topic_index++] += c;
        }
        token_topic_count_map_[token_hash] = token_topic_vec;
        ntokens_++;
    }
}

void Gibbs::import_corpus(std::string corpus_file) {
    std::ifstream corpus_stream(corpus_file);
    if (!corpus_stream.is_open()) {
        std::cout << "Unable to open file: " << corpus_file << std::endl;
        exit(1);
    }

    index_t ii(0);
    std::string line;
    std::cout << "Importing word counts for new docs..." << std::endl;
    while (getline(corpus_stream, line)) {
        std::vector<std::string> strs;
        boost::split(strs, line, boost::is_any_of("\t"));

        item_t itemid = boost::lexical_cast<item_t>(strs[0]);
        std::string word_count_str = strs[1];
        corpus_[itemid] = word_count_str;
        docid_list_.push_back(itemid);
        queue.push(ii++);
    }
}

void Gibbs::gibbs_sampling(size_t nthreads) {       
    std::cout << "Gibbs sampling..." << std::endl;
    boost::thread_group gibbs_threads;
    for (size_t t = 0; t < nthreads; ++t) {
        boost::thread *new_gibbs_thread = new boost::thread(&Gibbs::gibbs_sampler, this);
        gibbs_threads.add_thread(new_gibbs_thread);
    }
    gibbs_threads.join_all();
}

/* The actual sampling function */
void Gibbs::gibbs_sampler() {
    uint32_t this_thread = thread_index_++;
    // make a local copy of the topic count map, as it will be modified during sampling
    token_topic_count_map_t local_token_topic_count_map;
    for (token_topic_count_map_t::iterator it_tcmap = token_topic_count_map_.begin(); it_tcmap != token_topic_count_map_.end(); it_tcmap++) {
        token_topic_count_t tmp_count(it_tcmap->second);
        local_token_topic_count_map[it_tcmap->first] = tmp_count;
        assert(local_token_topic_count_map[it_tcmap->first].size() == ntopics_);
    }

    std::stringstream this_file_name;
    this_file_name << output_file_dir_ << "_" << this_thread << ".txt";
    std::ofstream this_outfile(this_file_name.str());
    if (!this_outfile.is_open()) {
        std::cerr << "Unable to open output file: " << this_file_name.str() << std::endl;
        exit(1);
    }

    index_t ii;
    item_t item_id;
    while (queue.pop(ii)) {
        item_t item_id = docid_list_[ii];
        std::string word_count_str = corpus_[item_id];
        // parse the json string of word counts
        std::stringstream ss;
        ss << word_count_str;
        boost::property_tree::ptree pt;
        boost::property_tree::read_json(ss, pt);

        // Bag of words
        std::map<token_t, count_t> current_bow;
        BOOST_FOREACH(boost::property_tree::ptree::value_type &v, pt) {
            assert(v.first.empty());
            boost::property_tree::ptree& pt2 = v.second;
            boost::property_tree::ptree::const_iterator it = pt2.begin();
            token_t token_hash = it->second.get_value<token_t>();
            it++;
            current_bow[token_hash] = it->second.get_value<count_t>();
        }

        // output an empy array if there is no word count
        if (current_bow.size() == 0) {
            this_outfile << item_id << "\t[]" << std::endl;
            continue;
        }
        // 1. init. global topic count for the whole corpus
        // make a local copy of data
        doc_topic_count_t local_global_topic_counts(global_topic_counts_);
        // 2. init doc_topic_count for the current document
        // make a local copy of data
        doc_topic_count_t current_doc_topic_count(ntopics_, 0);
        // 3. init. topic assignment for each word to initial value -1
        assignment_map_t assignment_map;
        for (std::map<token_t, count_t>::iterator it_bow = current_bow.begin(); it_bow != current_bow.end(); it_bow++) {
            // init the assignment to a number larger than num of topics
            assignment_t current_word_assignment(it_bow->second, ntopics_+1000);
            assignment_map[it_bow->first] = current_word_assignment;
        }
        // 4. iterate for burnin times, Eqn [5] in Finding Scientific Topics
        //size_t iteration = 4;
        std::vector<double> final_doc_topic_count(ntopics_, ALPHA_);
        for (size_t ite = 0; ite <= burnin_; ite++) {
            int nchanges = 0;
            for (assignment_map_t::iterator it_asg_map = assignment_map.begin();
                 it_asg_map != assignment_map.end();
                 it_asg_map++) {
                const token_t& current_hash = it_asg_map->first;
                assignment_t& current_hash_assignment = it_asg_map->second;
                for (size_t idx = 0; idx < current_hash_assignment.size(); idx++) {
                    index_t asg = current_hash_assignment[idx];
                    index_t old_asg = asg;

                    if (asg < ntopics_) {
                        local_token_topic_count_map[current_hash][asg] = 
                            local_token_topic_count_map[current_hash][asg] > 0 ? local_token_topic_count_map[current_hash][asg] - 1 : 0;
                        current_doc_topic_count[asg] = current_doc_topic_count[asg] > 0 ? current_doc_topic_count[asg] - 1 : 0;
                        local_global_topic_counts[asg] = local_global_topic_counts[asg] > 0 ? local_global_topic_counts[asg] - 1 : 0;
                    }
                    // Compute topic distribution at current step
                    std::vector<double> pz(ntopics_, 0.0);
                    for (size_t t = 0; t < ntopics_; t++) {
                        double n_dt = current_doc_topic_count[t];
                        double n_wt = local_token_topic_count_map[current_hash][t];
                        double n_t = local_global_topic_counts[t];
                        pz[t] = (ALPHA_ + n_dt) * (BETA_ + n_wt) / (BETA_ * ntokens_ + n_t);
                    }
                    // Sample from the multinomial distribution pz
                    asg = multinomial(pz);
                    local_global_topic_counts[asg]++;
                    local_token_topic_count_map[current_hash][asg]++;
                    current_doc_topic_count[asg]++;

                    if (old_asg != asg) {
                        current_hash_assignment[idx] = asg;
                        nchanges++;
                    }
                }
            } // assignment_map
            if (ite >= burnin_ || nchanges == 0) {
                // save the topic distribution to a global variable
                std::transform(final_doc_topic_count.begin(), final_doc_topic_count.end(), current_doc_topic_count.begin(), final_doc_topic_count.begin(), std::plus<double>());
                break;
            }
        } // for each iteration

        double sum_ = std::accumulate(final_doc_topic_count.begin(), final_doc_topic_count.end(), 0.0);
        std::transform(final_doc_topic_count.begin(), final_doc_topic_count.end(), final_doc_topic_count.begin(), 
                       std::bind2nd(std::divides<double>(), sum_));
        
        // output result to file
        this_outfile << item_id << "\t[";
        for (size_t i = 0; i < final_doc_topic_count.size() -1 ; i++) {
            this_outfile << final_doc_topic_count[i] << ", ";
        }
        this_outfile << final_doc_topic_count[final_doc_topic_count.size()-1] << "]" << std::endl;
    }
}
