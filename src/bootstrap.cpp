#include <cassert>

#include <cstddef>
using std::size_t;

#include <cstdlib>
using std::exit;

#include <cmath>
using std::sqrt;

#include <cstring>
using std::strcmp;

#include <string>
using std::string;
using std::stol;

#include <sstream>
using std::istringstream;

#include <iterator>
using std::istream_iterator;

#include <iostream>
using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::istream;

#include <fstream>
using std::ifstream;

#include <iomanip>
using std::setw;

#include <vector>
using std::vector;

#include <random>
using std::mt19937;
//using std::uniform_real_distribution;
using std::uniform_int_distribution;

#include <chrono>
using std::chrono::high_resolution_clock;

#include <functional>
using std::bind;
using std::ref;

static mt19937 rng;

#include <unistd.h> // for getopt

const char usagetext[] =
   "Usage:\n"
   "  bootstrap -h\n"
   "  bootstrap [-mrw][-X option_argument] filename|-\n"
   "   -h,      help\n"
   "   -m,      skip resampling step; report conventional mean and variance\n"
   "   -r,      dump the raw resampled data; report no statistical summary\n"
   "   -w,      weight average by values in first column (assuming bins <W_i> <X_i*W_i> <Y_i*W_i> ...)\n"
   "   -X num,  skip num initial rows of data";

bool use_weight = false;
bool use_raw = false;
bool use_mean_and_variance = false;

int main(int argc, char* argv[])
{
	
   // ##############################################################################
   // PARSE THE COMMAND LINE
   // ##############################################################################
	
   if (argc == 1 or (argc == 2 and strcmp(argv[1],"-h") == 0))
   {
      cerr << usagetext << endl;
      exit(0);
   }
   
   extern char *optarg;
   extern int optind;
   int c;
   size_t exclude = 0;
   while ((c = getopt (argc-1, argv, "mrwX:")) != -1)
   {
      if (c == 'm')
         use_mean_and_variance = true;
      else if (c == 'r')
         use_raw = true;
      else if (c == 'w')
         use_weight = true;
      else if (c == 'X')
      {
         long int Xval = stol(optarg);
         if (Xval < 1l)
         {
            cerr << "Error: flag -X takes a non-negative integer argument" << endl;
            exit(1);
         }
         exclude = Xval;
      }
      else if (c == '?')
      {
         if (optopt == 'X')
            cerr << "Error: option -X requires an argument." << endl;
         else if (isprint (optopt))
            cerr << "Error: unkown option -" << optopt << "." << endl;
         else
            cerr << "Error: unkown option." << endl;
         exit(1);
      }
      else
         exit(1);
   }

   if (use_weight and use_mean_and_variance)
   {
      cerr << "Error: options -m and -w cannot be used simulataneously." << endl;
      exit(1);
   }
   if (use_mean_and_variance and use_raw)
   {
      cerr << "Error: options -m and -r cannot be used simulataneously." << endl;
      exit(1);
   }
   
   string filename;         
   if (argc-optind == 1)
   {
      filename = argv[optind];
   }
   else 
   {
      cerr << "Error: command line arguments must end with a single valid filename or hyphen (-)" << endl;
      exit(1);
   }

   istream *fp = &cin;
   bool reading_from_file = false;
   ifstream fin;
   if (filename != "-")
   {
      reading_from_file = true;
      fin.open(filename);
      if (!fin.is_open()) { cerr << "File error: could not open `" << filename << "`" << endl; exit(1); }
      fp = &fin;
   }

	// ##############################################################################
	// SCAN FILE DATA; REPORT MEAN AND VARIANCE; END
	// ##############################################################################

   size_t words_per_line = 0;
   size_t line_count = 0;
   string nextline;
	
	vector<double> mean, variance, tmp;
	
	if (use_mean_and_variance)
	{
		while (true)
		{
			getline(*fp,nextline);
			if (!(*fp)) break;
			if (exclude != 0)
				exclude--;
			else
			{
				istringstream ss(nextline);
				istream_iterator<double> it(ss);
				istream_iterator<double> end;
				if (it == end) continue; // skip empty lines
				++line_count;
				
				size_t words = 0;
				tmp.resize(0);
				while (it != end) 
				{
					tmp.push_back(*it);
					words++;
					it++;
				}
				if (words_per_line == 0)
				{
					words_per_line = words;
					mean.assign(words_per_line,0.0);
					variance.assign(words_per_line,0.0);
				}
				else if (words_per_line != words)
				{
					cerr << "Formatting error: line " << line_count << " has " << words << " entries rather than " << words_per_line << endl;
					exit(1);
				}
				assert(mean.size() == tmp.size());
				assert(variance.size() == tmp.size());
				
				for (size_t col = 0; col < words_per_line; ++col)
				{
					double& m = mean[col];
					double& v = variance[col];
					const double y = tmp[col];
					const double delta = y-m;
					m += delta/line_count;
					if (line_count > 1)
						v += delta*(y-m);
				}
			}
		}
		
		if (line_count == 0)
		{
			if (reading_from_file)
				cerr << "Error: read in no lines of data from `" << filename << "`." << endl;
			else
				cerr << "Error: read in no lines of data from standard input." << endl;
			exit(1);
		}
		
		if (use_weight and words_per_line < 2)
		{
			cerr << "Error: need at least two columns of data with -w flag" << endl;
			exit(1);
		}
		
		cerr << "Read in " << line_count << " lines of " << words_per_line << " entries each." << endl;
		
		cout.precision(12);
		for (size_t col = (use_weight ? 1 : 0); col < words_per_line; ++col)
		{
			const double dm = sqrt(variance[col]/(line_count-1));
			cout << setw(20) << mean[col] << setw(20) << dm;
		}
		cout << endl;
		exit(0);
	}
   		
   // ##############################################################################
   // READ AND STORE FILE DATA
   // ##############################################################################
	
   vector<double> data;  
   while (true)
   {
      getline(*fp,nextline);
      if (!(*fp)) break;
      if (exclude != 0)
         exclude--;
      else
      {
         istringstream ss(nextline);
         istream_iterator<double> it(ss);
         istream_iterator<double> end;
         if (it == end) continue; // skip empty lines
         ++line_count;
			
         size_t words = 0;
         while (it != end) 
         {
            data.push_back(*it);
            words++;
            it++;
         }
         if (words_per_line == 0)
            words_per_line = words;
         else if (words_per_line != words)
         {
            cerr << "Formatting error: line " << line_count << " has " << words << " entries rather than " << words_per_line << endl;
            exit(1);
         }
      }
   }
    
   if (line_count == 0)
   {
      if (reading_from_file)
         cerr << "Error: read in no lines of data from `" << filename << "`." << endl;
      else
         cerr << "Error: read in no lines of data from standard input." << endl;
      exit(1);
   }
	
   if (use_weight and words_per_line < 2)
   {
      cerr << "Error: need at least two columns of data with -w flag" << endl;
      exit(1);
   }

   cerr << "Read in " << line_count << " lines of " << words_per_line << " entries each." << endl;

   assert(data.size() == words_per_line*line_count);

   // ##############################################################################
   // PERFORM BOOTSTRAP RESAMPLING
   // ##############################################################################

   rng.seed(mt19937::result_type(high_resolution_clock::now().time_since_epoch().count()));

   mean.assign(words_per_line,0);
   variance.assign(words_per_line,0);
   auto choose_row = bind(uniform_int_distribution<size_t>(0,line_count-1),rng);
   //auto choose_row = bind(uniform_int_distribution<size_t>(0,line_count-1),ref(rng));
   
   const size_t resamplings = 1000;
   for (size_t n = 1; n <= resamplings; ++n)
   {
      tmp.assign(words_per_line,0);
      for (size_t l = 0; l < line_count; ++l)
      {
         const size_t row = choose_row();
         for (size_t col = 0; col < words_per_line; ++col)
         {
            const size_t indx = row*words_per_line+col;
            assert(indx < data.size());
            const double x = data[indx];
            tmp[col] += x;
         }
      }
      if (use_raw)
      {
         for (size_t col = 0; col < words_per_line; ++col)
            cout << setw(20) << tmp[col]/line_count;
         cout << endl;
      }
      else
      {
         // See "Online algorithm"
         // https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
         for (size_t col = (use_weight ? 1 : 0); col < words_per_line; ++col)
         {
            double& m = mean[col];
            double& v = variance[col];
            const double y = tmp[col]/(use_weight ? tmp[0] : line_count);
            const double delta = y-m;
            m += delta/n;
            if (n > 1)
               v += delta*(y-m);
         }
      }
   }
    
   // ##############################################################################
   // REPORT MEAN AND VARIANCE OF THE BOOTSTRAP SAMPLES
   // ##############################################################################
	
   if (!use_raw)
   {
      bool sampling_problem = false;
      cout.precision(12);
      tmp.assign(words_per_line,0);
		
      {
         size_t col = 0;
         for (auto value : data)
         {
            tmp[col] += value;
            col = (col+1)%words_per_line;
         }
      }
        
      for (size_t col = (use_weight ? 1 : 0); col < words_per_line; ++col)
      {
         const double m = tmp[col]/(use_weight ? tmp[0] : line_count);
         const double dm = sqrt(variance[col]/resamplings);
         if (fabs(mean[col] - m) > dm) 
            sampling_problem = true;
         cout << setw(20) << m << setw(20) << dm;
      }
      cout << endl;
      if (sampling_problem)
         cerr << "Warning: potential sampling problem." << endl;
 	
      return 0;
   }
}