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

#include "gibbs.hpp"
#include <boost/program_options.hpp>

using namespace std;

int main(int argc, char** argv)
{
    string token_topic_count_file;
    string corpus_file;
    string output_file;
    count_t ntopics = 10;
    count_t nthreads = 2;
    double ALPHA = 1.0;
    double BETA = 0.1;
	count_t burn_in = 10;

    namespace po = boost::program_options;
    po::options_description desc("Gibbs Topic Inference - Allowed Options");
    desc.add_options()
        ("help,h", "produce this help message")
        ("corpus,c", po::value<std::string>(&corpus_file)->required(), "required, the corpus of word counts for which to infer topics")
        ("token_topic_count,t", po::value<std::string>(&token_topic_count_file)->required(), "required, the topic counts of all tokens from the topic model")
        ("output,o", po::value<std::string>(&output_file)->required(), "required, output file prefix for inferred topics")
        ("ntopics,n", po::value<count_t>(&ntopics), "Number of topics, default 10")
        ("nthreads,r", po::value<count_t>(&nthreads), "Number of threads, default 2")
        ("alpha,a", po::value<double>(&ALPHA), "Alpha, default 1.0")
        ("beta,b", po::value<double>(&BETA), "Beta, default 0.1")
        ("burnin,u", po::value<count_t>(&burn_in), "Burn-in iterations, default 10")
        ;

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        if (vm.count("help")) {
            cout << desc << "\n";
            return 1;
        }
        po::notify(vm);    
    } catch(po::error& e) { 
        std::cerr << "ERROR: " << e.what() << "\n\n"; 
        std::cerr << desc << std::endl; 
        return 1; 
    } 

    cout << "Parameters:";
    cout << "\n\tTopic model:\t" << token_topic_count_file;
    cout << "\n\tCorpus:\t" << corpus_file;
    cout << "\n\tOutput file:\t" << output_file;
    cout << "\n\tNumber of topics:\t" << ntopics;
    cout << "\n\tNumber of threads:\t" << nthreads;
    cout << "\n\tAlpha:\t\t" << ALPHA << "\n\tBeta:\t\t" << BETA;
	cout << "\n\tBurn-in:\t\t" << burn_in<< std::endl;

    Gibbs gb(output_file, ntopics, ALPHA, BETA, burn_in);
    gb.import_token_topic_count(token_topic_count_file);
    gb.import_corpus(corpus_file);
    gb.gibbs_sampling(nthreads);

    return 0;
}
