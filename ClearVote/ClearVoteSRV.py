import csv
import sys

cvr_file = sys.argv[1]
first_candidate_col = int(sys.argv[2])
max_score = int(sys.argv[3])
num_candidates = int(sys.argv[4])

print "Cast Vote Record File: " + cvr_file
print "First Candidate Column: " + str(first_candidate_col)
print "MaxScore: " + str(max_score)
print "NumCandidates: " + str(num_candidates)

reader = csv.reader(open(cvr_file, 'Urb'))
reader.next()

score_range = range(max_score + 1)
candidate_range = range(num_candidates)

candidate_score = []
for candidate in candidate_range:
	candidate_score.append(0)

preference_total = []
for c1 in candidate_range:
	preference_total.append([])
	for c2 in candidate_range:
		preference_total[c1].append(0)

print "Candidate Scores: " + str(candidate_score)
print "Candidate Preferences: " + str(preference_total)

for row in reader:
	print "Input row" + str(row)
	ballot_scores = []
	for candidate in candidate_range:
		ballot_scores.append(0)
		for score_tally in score_range:
			if row[first_candidate_col + candidate * (max_score + 1) + score_tally] == '1':
				ballot_scores[candidate] = score_tally
	print "Ballot scores: " + str(ballot_scores)
	for c1 in candidate_range:
		candidate_score[c1] += ballot_scores[c1]
		for c2 in candidate_range:
			if ballot_scores[c1] > ballot_scores[c2]:
				preference_total[c1][c2] += 1

print "Final scores: " + str(candidate_score)
print "Preference totals: " + str(preference_total)

