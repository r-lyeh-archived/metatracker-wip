// simple sync app

#include <iostream>
#include <thread>

#include <vector>
#include <string>
#include <iostream>
#include <sstream>

std::vector<std::string> split( const std::string &input, char sep ) {
    std::istringstream ss(input);
    std::vector<std::string> words;
    std::string word;
    while( std::getline(ss, word, sep) )
        words.push_back( word );
    return words;
}

std::string song = R"*(
Are you going to Scarborough Fair?
Parsley, sage, rosemary, and thyme;
Remember me to the one who lives there,
For once she was a true love of mine.
Tell her to make me a cambric shirt,
Parsley, sage, rosemary, and thyme;
Without a seam or needlework,
Then she'll be a true lover of mine.
Tell her to wash it in yonder well,
Parsley, sage, rosemary, and thyme;
Where never spring water or rain ever fell,
And she shall be a true lover of mine.
Tell her to dry it on yonder thorn,
Parsley, sage, rosemary, and thyme;
Which never bore blossom since Adam was born,
Then she shall be a true lover of mine.
Now he has asked me questions three,
Parsley, sage, rosemary, and thyme;
I hope he'll answer as many for me
Before he shall be a true lover of mine.
Tell him to buy me an acre of land,
Parsley, sage, rosemary, and thyme;
Between the salt water and the sea sand,
Then he shall be a true lover of mine.
Tell him to plough it with a ram's horn,
Parsley, sage, rosemary, and thyme;
And sow it all over with one pepper corn,
And he shall be a true lover of mine.
Tell him to sheer't with a sickle of leather,
Parsley, sage, rosemary, and thyme;
And bind it up with a peacock feather.
And he shall be a true lover of mine.
Tell him to thrash it on yonder wall,
Parsley, sage, rosemary, and thyme,
And never let one corn of it fall,
Then he shall be a true lover of mine.
When he has done and finished his work.
Parsley, sage, rosemary, and thyme:
Oh, tell him to come and he'll have his shirt,
And he shall be a true lover of mine.)*";

#include <chrono>
void sleep4( double seconds ) {
    std::this_thread::sleep_for( std::chrono::milliseconds( int(seconds * 1000) ));
}

int main() {

    if(0) {
        auto words = split( song, ' ' );
        for( unsigned it = 0, end = words.size(); it < end; ++it ) {
            std::cout << (it+1) << ") " << words[it] << std::endl;
            double delay = strlen(words[it].c_str()) * 0.075;
            char ch = words[it].back() != '"' ? words[it].back() : words[it][ words[it].size() - 2 ];
            switch( ch ) {
                break; default : sleep4( delay );
                break; case '?':
                       case ',': sleep4( delay + 0.5 );
                break; case '.': sleep4( delay + 2.0 );
            }
        }
        return 0;
    }

    int t0 = 0, t1 = 1000;
    for(;;) {
        t0 ++;
        t0 %= t1;
        printf("metatracker %d %d\n", t0, t1 );
        printf("random %d %d\n", rand(), rand());
        std::this_thread::sleep_for( std::chrono::milliseconds(1) );
    }
}
