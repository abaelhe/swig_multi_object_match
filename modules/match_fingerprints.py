#!/usr/bin/env python
#coding:utf-8

"""
@Author: Abael He<abaelhe@icloud.com>
@file: match_fingerprints.py
@time: 03/12/2018 19:36
@license: All Rights Reserved, Abael.com
"""

import os, sys, itertools, collections
from heapq import nlargest
from collections import defaultdict


def match_fingerprints(bigdict, fingerprints, n_largest=5, result_dict=None):
    ds =defaultdict(lambda: defaultdict(int)) if result_dict is None else result_dict
    for hash_id, offset in fingerprints:
        records = bigdict.get(hash_id)
        if records is None:
            continue

        diffs = map(lambda x: x - offset, records.offsets)  # offset: 2byte int
        songids = records.songids  # song_id: 4byte int
        for i in range(records.num):
            ds[songids[i]][diffs[i]] += 1

    songid_and_tickets = tuple((k, max(v.values())) for k, v in (ds.popitem() for i in range(len(ds))))
    songid_and_tickets_nlargest = zip(*nlargest(n_largest, songid_and_tickets, lambda item: item[1]))
    del songid_and_tickets
    return songid_and_tickets_nlargest
