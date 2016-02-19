#!/usr/bin/env python
import pprint
class AutoCorrect:
    def __init__(self):
        pass;

    def ngrams(self, line, size=3):
        'split a string int ngrams of size specified'
        return [line[i:i+size] for i in range(0, len(line)-size+1)]

    def suggestions(self, usercmd, cmdlist, count=5):
        '''
        Return a list if suggestions from cmdlist that is similar to usercmd
        '''
        options=[]
        usercmd=usercmd.lower().strip()
        # get the string deistances
        for cmd in cmdlist:
            d=self.stringdistance(cmd.lower(), usercmd)
            options.append([d, 0, cmd])
        
        #options.sort(key=lambda x: x[0])
        #pprint.pprint(options)
        grams = self.ngrams(usercmd, 3 if len(usercmd) > 4 else 2)
        if len(grams) > 0:
            for o in options:
                o[1] = sum([1 for piece in grams if piece in o[2].lower()])

        # remove totally off items
        options = [o for o in options if (o[0] < len(usercmd) - 1) and (len(grams) < 3 or  o[1] >= 0.3*len(grams))]
         
        options.sort(key=lambda x: x[0] - 2*x[1])
        #pprint.pprint(options)
        return [o[2] for o in options[:count]]

    def stringdistance(self, string1, string2):
        '''
        returns the distance between 2 strings
        https://en.wikipedia.org/wiki/Levenshtein_distance
        '''
        w=1; s=1; a=1; d=0
        len1 = len(string1)
        len2 = len(string2)
        row0 = [0] * (len2 + 1)
        row1 = [0] * (len2 + 1)
        row2 = [0] * (len2 + 1)

        for j in range(0,len2+1):
            row1[j] = j * a

        for i in range(0,len1):
            row2[0] = (i + 1) * d
            for j in range(0,len2):
                # substitution
                row2[j + 1] = row1[j] + s * (string1[i] != string2[j])
                # swap
                if (i > 0 and j > 0 and string1[i - 1] == string2[j] and
                       string1[i] == string2[j - 1] and
                        row2[j + 1] > row0[j - 1] + w):
                    row2[j + 1] = row0[j - 1] + w
                # deletion
                if (row2[j + 1] > row1[j + 1] + d):
                    row2[j + 1] = row1[j + 1] + d
                # insertion
                if (row2[j + 1] > row2[j] + a):
                    row2[j + 1] = row2[j] + a

            dummy = row0
            row0 = row1
            row1 = row2
            row2 = dummy

        return row1[len2]
