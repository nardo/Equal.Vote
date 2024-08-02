import csv
from collections import Counter

def adjust_ranks(ranks):
    adjusted_ranks = [rank for rank in ranks if rank.lower() != 'skipped' and rank.lower() != 'write-in']
    res = []
    [res.append(x) for x in adjusted_ranks if x not in res]
    return res + [' '] * (4 - len(res))

def summarize_ballots(file_path):
    with open(file_path, newline='') as csvfile:
        reader = csv.reader(csvfile)
        header = next(reader)  # Skip the header row

        # Extract the last four columns (assuming these are the rank columns)
        ballot_ranks = [adjust_ranks(row[-4:]) for row in reader]
    
    # Create a string representation of each ballot ranking
    ballot_ranks_str = ['\t'.join(ballot) for ballot in ballot_ranks]
    
    # Count the unique ballot orderings
    ballot_counts = Counter(ballot_ranks_str)
    
    # Create a summary table
    summary_table = [{'Ballot Ranking': ranking, 'Count': count} for ranking, count in ballot_counts.items()]
    
    # Sort the summary table by count in descending order
    summary_table.sort(key=lambda x: x['Count'], reverse=True)
    
    # Print the summary table
    print("Count | Ballot Ranking")
    print("------------------------")
    for row in summary_table:
        print(f" {row['Count']}\t{row['Ballot Ranking']}")
    
    return summary_table

# Example usage
file_path = 'Alaska_08162022_HouseofRepresentativesSpecial.csv'
summary_table = summarize_ballots(file_path)