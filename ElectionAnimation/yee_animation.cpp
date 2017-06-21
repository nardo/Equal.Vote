#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <png.h>
#include <sys/time.h>
#include <unistd.h>
#include "smith_random.h"
#include "image.h"

#define PI 3.14159265358979323844

enum { MaxCandidates = 6, num_voters = 4096, score_range = 5, width = 256, height = 256 };

unsigned char winner_color[][3] = {
 { 255, 0, 0 },
  { 0, 0, 255 },
{ 255, 255, 0 },
 { 0, 255, 0 },
 { 0, 255, 255 },
};
int num_candidates = 2;

float voter_pos[num_voters][2];
float candidate_pos[MaxCandidates][2];

float initial_candidate_pos[][2] = {
// two candidates
{ 0.25, 0.25 },
{ 0.75, 0.75 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
// three candidates
{ 0.5, 0.99 },
{ 0.07, 0.25 },
{ 0.93, 0.25 },
{ 0, 0 },
{ 0, 0 },
// four candidates
{ 0, 0 },
{ 0, 1 },
{ 1, 1 },
{ 1, 0 },
{ 0, 0 },
// five candidates
{ 0.91, 0.70 },
{ 0.5, 0.99 },
{ 0.07, 0.25 },
{ 0.93, 0.25 },
{ 0.75, 0.75 },
};


float candidate_vel[][2] = {

{ 0.005, 0.025 },
{ -0.01, 0.0175 },
{ -0.025, -0.025 },
{ 0.035, - 0.01 },
{ 0.0025, - 0.001 },

};

int num_animation_steps = 256;

float voter_distance[num_voters][MaxCandidates];
float voter_distance_normalized[num_voters][MaxCandidates];
int scores[num_voters][MaxCandidates];
int scores321[num_voters][MaxCandidates];
int tally1[MaxCandidates];
int tally3[MaxCandidates];

int star_scores[num_voters][MaxCandidates];
int ranks[num_voters][MaxCandidates];
int irv_ballot_pos[num_voters];
int total_score[MaxCandidates];
int total_star_score[MaxCandidates];
int tally[MaxCandidates];

enum {
	MethodPlurality = 0,
	MethodIRV = 1,
	MethodScore = 2,
	MethodSTAR = 3,
	MethodVoronoi = 4,
	Method321 = 5,
	NumMethods
};

int winner[NumMethods];
int all_winners[height][width][NumMethods];


float randf(float range)
{
	return random() * (range / float(0x7fffffff));
}

void setup_voters(float gauss_std_deviation)
{
	//gauss_std_deviation = 1.5;
	//for(int i = 0; i < num_voters; i++)
	//{
	//	float theta = randf(2 * PI);
	//	float r = randf(gauss_std_deviation);
	//	voter_pos[i][0] = cos(theta) * r;
	//	voter_pos[i][1] = sin(theta) * r;
	//}

	for(int i = 0; i < num_voters; i++)
	{
		float theta = i * 2 * PI / float(num_voters);
		float r = RandRadialNormal() * gauss_std_deviation;
		voter_pos[i][0] = cos(theta) * r;
		voter_pos[i][1] = sin(theta) * r;
	}
}

float distance_offset(float *p1, float *offset, float *p2)
{
	float dx = p1[0] + offset[0] - p2[0];
	float dy = p1[1] + offset[1] - p2[1];
	
	return(sqrt(dx * dx + dy * dy));
}

float distance(float *p1, float *p2)
{
	float dx = p1[0] - p2[0];
	float dy = p1[1] - p2[1];
	
	return(sqrt(dx * dx + dy * dy));
}

void compute_ranks_and_scores(float pix_pos[2])
{
	// compute the distance to all the candidates
	for(int i = 0; i < num_voters; i++)
		for(int c = 0; c < num_candidates; c++)
			voter_distance[i][c] = distance_offset(voter_pos[i], pix_pos, candidate_pos[c]);

	// bubble sort all of the candidates into rank orders for each voter
	for(int i = 0; i < num_voters; i++)
	{
		ranks[i][0] = 0;
		for(int c = 1; c < num_candidates; c++)
		{
			ranks[i][c] = c;
			int bubble = c;
			while(bubble > 0 && voter_distance[i][ranks[i][bubble]] < voter_distance[i][ranks[i][bubble-1]])
			{
				ranks[i][bubble] = ranks[i][bubble-1];
				ranks[i][bubble-1] = c;
				bubble--;
			}
		}
		// now that the elements are sorted, computing normalized distance is trivial
		float min_distance = voter_distance[i][ranks[i][0]];
		float max_distance = voter_distance[i][ranks[i][num_candidates-1]];
		
		for(int c = 0; c < num_candidates; c++)
		{
			voter_distance_normalized[i][c] = (voter_distance[i][c] - min_distance) / (max_distance - min_distance);
			
			scores[i][c] = int((1 - voter_distance_normalized[i][c]) * (float(score_range) + 0.9999));
			if(voter_distance_normalized[i][c] < 0.2)
				scores321[i][c] = 1;
			else if(voter_distance_normalized[i][c] < 0.6)
				scores321[i][c] = 2;
			else
				scores321[i][c] = 3;
		}
		star_scores[i][ranks[i][0]] = score_range;
		star_scores[i][ranks[i][num_candidates - 1]] = 0;
		for(int r = 1; r < num_candidates - 1; r++)
		{
			star_scores[i][ranks[i][r]] = int((1 - voter_distance_normalized[i][ranks[i][r]]) * (float(score_range - 2) + 0.9999)) + 1;
		}
	}
}

void run_election(float pix_pos[2])
{
	for(int i = 0; i < num_voters; i++)
		irv_ballot_pos[i] = 0;
	
	int eliminated[num_candidates];
	for(int c = 0; c < num_candidates; c++)
	{
		eliminated[c] = 0;
		total_score[c] = 0;
		total_star_score[c] = 0;
		tally[c] = 0;
		tally1[c] = 0;
		tally3[c] = 0;
	}
	
	// compute the plurality and scorre winners
	for(int i = 0; i < num_voters; i++)
	{
		tally[ranks[i][0]]++;
		for(int c = 0; c < num_candidates; c++)
		{
			if(scores321[i][c] == 1)
				tally1[c]++;
			else if(scores321[i][c] == 3)
				tally3[c]++;
			
			total_score[c] += scores[i][c];
			total_star_score[c] += star_scores[i][c];
		}
	}
	
	// compute plurality and score winners
	winner[MethodPlurality] = 0;
	winner[MethodScore] = 0;
	winner[MethodVoronoi] = 0;

	float v_distance = distance(pix_pos, candidate_pos[0]);
	

	for(int c = 1; c < num_candidates; c++)
	{
		if(tally[c] > tally[winner[MethodPlurality]])
			winner[MethodPlurality] = c;
		if(total_score[c] > total_score[winner[MethodScore]])
			winner[MethodScore] = c;
		float d = distance(pix_pos, candidate_pos[c]);
		if(d < v_distance)
		{
			v_distance = d;
			winner[MethodVoronoi] = c;
		}
	}
	
	// run the IRV rounds:
	int irv_leader = winner[MethodPlurality];
	int candidates_remaining = num_candidates;
	
	while(tally[irv_leader] <= (num_voters / 2) && (candidates_remaining > 1))
	{
		// find the least number of first place votes
		int loser = irv_leader;
		for(int c = 0; c < num_candidates; c++)
			if(!eliminated[c] && tally[c] < tally[loser])
				loser = c;
		
		// eliminate c and add all of their voters next choices to the tally
		for(int i = 0; i < num_voters; i++)
		{
			if(irv_ballot_pos[i] < num_candidates)
			{
				if(ranks[i][irv_ballot_pos[i]] == loser)
				{
					do {
						++irv_ballot_pos[i];
					} while(irv_ballot_pos[i] < num_candidates && eliminated[ranks[i][irv_ballot_pos[i]]]);
					
					if(irv_ballot_pos[i] < num_candidates)
						tally[ranks[i][irv_ballot_pos[i]]]++;
				}
			}
		}
		eliminated[loser] = true;
		candidates_remaining--;
		
		for(int c = 0; c < num_candidates; c++)
			if(tally[c] > tally[irv_leader])
				irv_leader = c;
	}
	winner[MethodIRV] = irv_leader;

	// Compute the Star Voting winner
	
	int star_top_a = 0;
	int star_top_b = 1;
	
	for(int c = 2; c < num_candidates; c++)
	{
		if(total_star_score[c] > total_star_score[star_top_b])
		{
			if(total_star_score[star_top_b] > total_star_score[star_top_a])
				star_top_a = star_top_b;
			star_top_b = c;
		}
		else if(total_star_score[c] > total_star_score[star_top_a])
			star_top_a = c;
	}
	// now we have the top two
	int tally_a = 0, tally_b = 0;
	
	for(int i = 0; i < num_voters; i++)
	{
		if(star_scores[i][star_top_a] > star_scores[i][star_top_b])
			tally_a++;
		else if(star_scores[i][star_top_b] > star_scores[i][star_top_a])
			tally_b++;
	}
	winner[MethodSTAR] = tally_a > tally_b ? star_top_a : star_top_b;
	
	// Compute the 321 winner
	
	int semi_finalists[3];
	semi_finalists[0] = 0;
	semi_finalists[1] = 1;
	semi_finalists[2] = 2;
	for(int c = 3; c < num_candidates; c++)
	{
		int f = c;
		if(tally1[f] > tally1[semi_finalists[0]])
		{
			f = semi_finalists[0];
			semi_finalists[0] = c;
		}
		if(tally1[f] > tally1[semi_finalists[1]])
		{
			int t = semi_finalists[1];
			semi_finalists[1] = f;
			f = t;
		}
		if(tally1[f] > tally1[semi_finalists[2]])
			semi_finalists[2] = f;
	}
	int finalist1 = semi_finalists[0];
	int finalist2 = semi_finalists[1];
	if(tally3[semi_finalists[2]] < tally3[finalist1])
	{
		if(tally3[finalist1] < tally3[finalist2])
			finalist2 = semi_finalists[2];
		else
			finalist1 = semi_finalists[2];
	}
	else if(tally3[semi_finalists[2]] < tally3[finalist2])
		finalist2 = semi_finalists[2];
	int f1tally = 0;
	int f2tally = 0;
	for(int i = 0; i < num_voters; i++)
	{
		if(scores321[i][finalist1] < scores321[i][finalist2])
			f1tally++;
		else if(scores321[i][finalist2] < scores321[i][finalist1])
			f2tally++;
	}
	winner[Method321] = f1tally > f2tally ? finalist1 : finalist2;
}

image<width, height> img;

void build_election_image(int method)
{
	for(int y = 0; y < height; y++)
		for(int x = 0; x < width; x++)
			img.pixel(x, y, winner_color[all_winners[y][x][method]]);

	for(int c = 0; c < num_candidates; c++)
		img.draw_circle(candidate_pos[c][0] * width, candidate_pos[c][1] * height, 4, winner_color[c]);
}

void build_diff_image(int method)
{
	for(int y = 0; y < height; y++)
	{
		for(int x = 0; x < width; x++)
		{
			if(all_winners[y][x][method] == all_winners[y][x][MethodVoronoi])
			{
				img.data[y][x][0] = img.data[y][x][1] = img.data[y][x][2] = 0;
			}
			else
			{
				// how bad a choice did we make? Let's measure the distances to the two candidates:
				float pix_pos[2] = { float(x) / float(width), float(y) / float(height) };
				float d_delta = distance(pix_pos, candidate_pos[all_winners[y][x][method]]) - distance(pix_pos, candidate_pos[all_winners[y][x][MethodVoronoi]]);
				
				float h, s = 1, v;
				
				if(d_delta > 0.3333)
					d_delta = 0.3333;
				
				d_delta *= 3;
				
				h = 120 - d_delta * 120;
				if(d_delta > 0.1)
					v = 0.85 + 0.15 * d_delta;
				else
					v = 0.85 * d_delta * 10;
				
				/*if(d_delta < 0.05)
				{
					h = 120;
					v = d_delta / 0.05;
				}
				else
				{
					if(d_delta > 0.5)
						d_delta = 0.5;
					v = 1;
					h = 120 - ((d_delta - 0.05)/ 0.45) * 120;
				}*/
				
				hsv2rgb(h, s, v, img.data[y][x]);
			}
		}
	}
	for(int c = 0; c < num_candidates; c++)
		img.draw_circle(candidate_pos[c][0] * width, candidate_pos[c][1] * height, 4, winner_color[c]);
}

image<512, 512> big_image;
void draw_voters(int cx, int cy, float multiplier)
{
	for(int i = 0; i < num_voters; i++)
	{
		unsigned int vy = voter_pos[i][1] * multiplier + cy;
		unsigned int vx = voter_pos[i][0] * multiplier + cx;
		big_image.draw_circle(vx, vy, 2, white);
	}
}

char filename[32];

void run_election_frame()
{
	for(int y = 0; y < height; y++)
	{
		for(int x = 0; x < width; x++)
		{
			float pix_pos[2] = { float(x) / float(width), float(y) / float(height) };
			compute_ranks_and_scores(pix_pos);
			run_election(pix_pos);
			for(int m = 0; m < NumMethods; m++)
			{
				all_winners[y][x][m] = winner[m];
			}
		}
	}
}

void render_explanatory_anim_frames()
{
	big_image.clear(black);
	big_image.draw_box(128, 128, 256, 256, white);
	draw_voters(128, 128, 256);
	big_image.writeImage("boxtext.png", "This is my test image");
	num_candidates = 2;
	
	candidate_pos[0][0] = initial_candidate_pos[0][0];
	candidate_pos[0][1] = initial_candidate_pos[0][1];
	candidate_pos[1][0] = initial_candidate_pos[1][0];
	candidate_pos[1][1] = initial_candidate_pos[1][1];
	big_image.clear(black);
	big_image.draw_box(128, 128, 256, 256, white);
	for(int c = 0; c < 2; c++)
		big_image.draw_circle(candidate_pos[c][0] * width + 128, candidate_pos[c][1] * height + 128, 4, winner_color[c]);
	
	int start[2] = { 220, 180 };
	int end[2] = { 380, 300 };
	
	big_image.writeImage("two_candidates.png", "This is my test image");
	for(int t = 0; t <= 10; t += 1)
	{
		big_image.clear(black);
		big_image.draw_box(128, 128, 256, 256, white);
		for(int c = 0; c < 2; c++)
			big_image.draw_circle(candidate_pos[c][0] * width + 128, candidate_pos[c][1] * height + 128, 4, winner_color[c]);
		big_image.draw_circle(start[0] + (end[0] - start[0]) * t / 10, start[1] + (end[1] - start[1]) * t / 10, 4, white);
		sprintf(filename, "two_cand_and_voter_%d.png", t);
		big_image.writeImage(filename, "This is my test image");
	}

	// render explosion of voters
	
	int num_explosion_frames = 75;
	
	for(int frame = 0; frame < num_explosion_frames; frame++)
	{
		int multiplier = 256 * frame / (num_explosion_frames - 1);
		big_image.clear(black);
		draw_voters(220, 180, multiplier);
		for(int c = 0; c < 2; c++)
			big_image.draw_circle(candidate_pos[c][0] * width + 128, candidate_pos[c][1] * height + 128, 4, winner_color[c]);
		big_image.draw_box(128, 128, 256, 256, white);
		sprintf(filename, "explosion_%d.png", frame);
		big_image.writeImage(filename, "This is my test image");
		
	}

	run_election_frame();
	
	// render frames of the paint animation
	int frame_step = 512 + 71;
	int last = false;
	
	for(int frame = 0; ; frame++)
	{
		int pix_pos = frame * frame_step;
		int row = pix_pos >> 8;
		int col = pix_pos & 0xFF;
		if(row > 255)
		{
			if(last)
				break;
			last = true;
			col = 255;
			row = 255;
		}
		
		big_image.clear(black);
		
		for(int y = 0; y < row; y++)
			for(int x = 0; x < width; x++)
				big_image.pixel(x + 128, y + 128, winner_color[all_winners[y][x][MethodVoronoi]]);
		for(int x = 0; x < col; x++)
			big_image.pixel(x + 128, row + 128, winner_color[all_winners[row][x][MethodVoronoi]]);
		for(int c = 0; c < 2; c++)
			big_image.draw_circle(candidate_pos[c][0] * width + 128, candidate_pos[c][1] * height + 128, 4, winner_color[c]);
		if(!last)
			big_image.draw_circle(128 + col, 128 + row, 4, white);
		big_image.draw_box(128, 128, 256, 256, white);
		sprintf(filename, "paint_bkgnd_%d.png", frame);
		big_image.writeImage(filename, "This is my test image");
	}
	
	frame_step = 5;

	// now render frames with the exploded voters
	int frame_index = 0;
	for(int frame = 0; ; frame++)
	{
		int row = frame_index >> 8;
		int col = frame_index & 0xFF;
		if(row > 255)
		{
			if(last)
				break;
			last = true;
			col = 255;
			row = 255;
		}
		frame_index += frame_step;
		frame_step ++;
		
		big_image.clear(black);
		
		for(int y = 0; y < row; y++)
			for(int x = 0; x < width; x++)
				big_image.pixel(x + 128, y + 128, winner_color[all_winners[y][x][MethodPlurality]]);
		for(int x = 0; x < col; x++)
			big_image.pixel(x + 128, row + 128, winner_color[all_winners[row][x][MethodPlurality]]);

		float fpos[2] = { float(col) / float(width), float(row) / float(height) };
		compute_ranks_and_scores(fpos);
		
		for(int i = 0; i < num_voters; i++)
		{
			unsigned int vy = voter_pos[i][1] * 256 + 128 + row;
			unsigned int vx = voter_pos[i][0] * 256 + 128 + col;
			big_image.draw_circle(vx, vy, 2, white);
			big_image.pixel(vx, vy, winner_color[ranks[i][0]]);
		}
		big_image.draw_circle(128 + col, 128 + row, 4, white);

		for(int c = 0; c < 2; c++)
			big_image.draw_circle(candidate_pos[c][0] * width + 128, candidate_pos[c][1] * height + 128, 4, winner_color[c]);

		big_image.draw_box(128, 128, 256, 256, white);
		sprintf(filename, "paint_expl_%d.png", frame);
		big_image.writeImage(filename, "This is my test image");
	}
	
}

int main(int argc, const char **argv)
{
	InitRand(0);
	float gauss = 0.5;
	setup_voters(gauss);
	img.clear(black);
	//render_explanatory_anim_frames();
	
	for(num_candidates = 3; num_candidates <= 5; num_candidates++)
	{
		// initialize candidate positions
		int index = (num_candidates - 2) * 5;
		for(int c = 0; c < num_candidates; c++)
		{
			candidate_pos[c][0] = initial_candidate_pos[index + c][0];
			candidate_pos[c][1] = initial_candidate_pos[index + c][1];
		}
		
		for(int frame = 0; frame < num_animation_steps; frame++)
		{
			run_election_frame();

			for(int method = 0; method < NumMethods; method++)
			{
				build_election_image(method);
				sprintf(filename, "yee_%d_%d.png", (num_candidates - 3) * num_animation_steps + frame, method);
				img.writeImage(filename, "This is my test image");
				if(method != MethodVoronoi)
				{
					build_diff_image(method);
					sprintf(filename, "yee_diff_%d_%d.png", (num_candidates - 3) * num_animation_steps + frame, method);
					img.writeImage(filename, "This is my test image");
				}
			}
			for(int c = 0; c < num_candidates; c++)
			{
				for(int i = 0; i < 2; i++)
				{
					candidate_pos[c][i] += candidate_vel[c][i];
					if(candidate_pos[c][i] < 0)
					{
						candidate_pos[c][i] = -candidate_pos[c][i];
						candidate_vel[c][i] = -candidate_vel[c][i];
					}
					else if(candidate_pos[c][i] > 1)
					{
						candidate_pos[c][i] = 2 - candidate_pos[c][i];
						candidate_vel[c][i] = -candidate_vel[c][i];
					}
				}
			}
		}
	}
	
	return 0;
}

