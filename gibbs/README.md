Gibbs Sampling for Topic Models
==========


License
-------

This is free software licensed under the Apache 2.0 License. See LICENSE.txt for details.

Usage
-------
This Gibbs sampling code infers topic vectors for new documents based on the given topic model.

*  -h [ --help ]                produce this help message
*  -c [ --corpus ]              required, word counts of documents for which to infer topics. <br> Format: document\_id \\t [[integer\_token\_hash2, count], [integer\_token\_hash2, count], ...]. <br> Example: 9548e888-e318-4266-b520-3b15bc01214c    [[792485005, 102], [2939649605, 66], [407529294, 57], [3329289074, 57], [3007176478, 36]]
*  -t [ --token\_topic\_count ] required, the topic counts of all tokens from the topic model. <br> Format: interger\_token\_hash \\t [count1, count2, ..., countN]. <br> Example: 1367540221      [0, 20418, 10, 1215, 0, 1865, 973, 72, 459, 0], assuming there are 10 topics.
*  -o [ --output ] arg          required, the output file for inferred topics
*  -n [ --ntopics ]             Number of topics, default 100
*  -r [ --nthreads ]            Number of threads, default 2
*  -a [ --alpha ] arg           Alpha, default 1.0
*  -b [ --beta ] arg            Beta, default 0.1
*  -u [ --burnin ] arg          Burn-in iterations, default 10
