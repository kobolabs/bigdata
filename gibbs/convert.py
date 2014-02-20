import sys
import json
import itertools

import aaargh

app = aaargh.App(description="Manipulate graphlab data file")


@app.cmd(help="Convert Graphlab format input data to docid \\t json_array format")
def graphlab_input2json():
    for docid, g in itertools.groupby(sys.stdin, key=lambda x: x.strip().split()[0]):
        token_counts = []
        for line in g:
            token, count = map(int, line.strip().split()[1:])
            token_counts.append([token, count])
        print("{}\t{}".format(docid, json.dumps(token_counts)))


@app.cmd(help="Convert input from docid \\t json_array format to Graphlab format")
def json2graphlab_input():
    for line in sys.stdin:
        docid, json_str = line.split('\t')
        token_counts = json.loads(json_str)
        for token, count in token_counts:
            print("{}\t{}\t{}".format(docid, token, count))


@app.cmd(help="Convert graphlab output to itemid \\t json_array format")
def graphlab_output_2json():
    for line in sys.stdin:
        fields = line.split()
        print("{}\t{}".format(fields[0], json.dumps(map(int, fields[1:]))))


def main():
    app.run()

if __name__ == '__main__':
    main()
