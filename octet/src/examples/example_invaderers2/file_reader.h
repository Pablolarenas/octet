#ifndef file_reader_h
#define file_reader_h

#include <fstream>

namespace octet {
	class file_reader {
		static int level_values[3];
	    static std::string file = "level.txt";
		static void extract(std::string location) {
			std::ifstream input_files(location);
			if (input_files.bad()) {}
			else {
				for (int i = 0; i < 3; i++) {
					std::string buffer;
					std::getline(input_files, buffer, ',');
					level_values[i] = std::stoi(buffer);
				}
			}
		}
	public:
		static void set_up_totals(int& guards, int& traps, int& diamond) {
			extract(file);
			guards = level_values[0];
			traps = level_values[1];
			diamond = level_values[2];
		}

	};

}

#endif

