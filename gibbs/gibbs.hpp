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

#ifndef GIBBS_HPP_
#define GIBBS_HPP_

#include <string>
#include <map>
#include <vector>

typedef std::size_t index_t;
typedef std::size_t token_t;
typedef std::size_t count_t;
typedef std::string item_t;
typedef std::vector<double> topic_vec_t;
typedef std::vector<count_t> doc_topic_count_t;
typedef std::vector<count_t> token_topic_count_t;
typedef std::map<token_t, token_topic_count_t > token_topic_count_map_t;

typedef std::vector<index_t> assignment_t;
typedef std::map<token_t, assignment_t> assignment_map_t;

class Gibbs {
public:
    Gibbs(std::string outputfile, count_t ntopics = 100, double alpha = 1.0, double beta = 0.1, count_t burn_in = 10);
    ~Gibbs();
    void import_token_topic_count(std::string token_topic_count_file);
    void import_corpus(std::string corpuss_file);
    void gibbs_sampling(size_t nthreads);

private:
    std::string output_file_dir_;
    count_t ntopics_;
    count_t ntokens_;
    double ALPHA_;
    double BETA_;
    count_t burnin_;
    
    std::map<item_t, std::string> corpus_;
    std::vector<item_t> docid_list_;
    token_topic_count_t global_topic_counts_;
    token_topic_count_map_t token_topic_count_map_;

    void gibbs_sampler();
};

#endif
