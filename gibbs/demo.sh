#!/bin/bash -ex

# Specifiy the Graphlab LDA executable as well as the Gibbs sampler
LDA_EXEC=./cgs_lda
GIBBS_DIR=./bigdata/gibbs

function lda () {
    # Download the Daily Kos data
    wget https://graphlabapi.googlecode.com/files/daily_kos.tar.bz2
    tar -xjf daily_kos.tar.bz2

    # Run LDA to train a topic model
    $LDA_EXEC --corpus ./daily_kos/tokens --dictionary ./daily_kos/dictionary.txt \
        --ntopics 10 --doc_dir ./doc_counts --word_dir ./word_counts --ncpus 8 --burnin 60
}

function gibbs() {
    # Convert the model (word-topic counts) from Graphlab output to suitable formats
    cat word_counts.* | python $GIBBS_DIR/convert.py graphlab_output2json | sort -k1,1 -g > lda_word_counts

    # Assume we are going to infer the topics for the same sets of documents again, 
    # but using the pre-trained topic model and this Gibbs sampler code.
    # Before that we need to convert the data to suitable format.
    cat daily_kos/tokens/doc_word_count.tsv | python $GIBBS_DIR/convert.py graphlab_input2json > doc_corpus

    # Run gibbs sampler
    $GIBBS_DIR/gibbs -c doc_corpus -t lda_word_counts -o inferred_topics -r 4 -u 10

    # Gather the inferred topics
    cat inferred_topics_* | sort -k1,1 -g > inferred_topics.txt
    rm inferred_topics_*
}

case "$1" in
    lda) lda
        ;;
    gibbs) gibbs
        ;;
    *) echo "Acceptable args: lda, gibbs"
        ;;
esac
