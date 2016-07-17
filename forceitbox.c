#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/sha.h>
#include <libgen.h>


typedef struct {
	time_t start;
	size_t duration;
	size_t counter;
	size_t limit;
} benchmark_t;


static void
benchmark_start (benchmark_t *bench)
{
	memset(bench, 0, sizeof(*bench));
	bench->start = time(NULL);
	bench->limit = 100;
}

static void
benchmark_tick (benchmark_t *bench)
{
	if( bench->counter++ >= bench->limit )
	{
		time_t now = time(NULL);
		int duration = now - bench->start;
		if( duration < 2 ) {
			bench->limit *= 1.5;
			return;
		}
		printf("%ld/s\n", bench->counter / duration);
		bench->counter = 0;
		bench->start = now;
		bench->duration += duration;
	} 
}

// Used to emulate how PHP converts doubles to strings...
static inline int
strip_trailing_zeroes( int n, char *buf )
{
	while( --n ) {
		char old = buf[n];
		if( old == '0' || old == '.' ) {
			buf[n] = 0;
			if( old == '.' )
				break;
			continue;
		}
		break;
	}
	return n;
}

static inline void
conv_to_hex( unsigned char *str, size_t len, char *out )
{
	static const char * hex = "0123456789abcdef";
	for( size_t N = 0; N < len; N++ )
	{
		unsigned char raw = str[N];
		*out++ = hex[ (raw >> 4) & 0xF ];
		*out++ = hex[ raw & 0xF ];
	}
}

/*
Equivalent to...
$genSalt3 = $genSalt2 / 1000;
  $genSalt4 = $genSalt3 * $genSalt;
  $salt = sha1($genSalt4);
  $spacer = "+";
   //generate roll 
	$pick = mt_rand(0, 10000);
	
	$pick2 = $pick / 100;
	$proof = sha1($salt.$spacer.$pick2);
*/
static int
bruteforcer (size_t genSalt, size_t genSalt2, unsigned char *search_hash, unsigned char *found_hash, double *found_pick)
{
	SHA_CTX salt_ctx;
	unsigned char salt_raw[20];
	char proof_raw[60];
	double genSalt3, genSalt4;
	char genSalt4_buf[30];

	proof_raw[40] = '+';

	// Calculate shitty salt number
	genSalt3 = genSalt2 / 1000.0;
	genSalt4 = genSalt3 * genSalt;
	int n = sprintf(genSalt4_buf, "%.1lf", genSalt4);
	n = strip_trailing_zeroes(n, genSalt4_buf);

	// Then hash it			
	SHA1_Init(&salt_ctx);
	SHA1_Update(&salt_ctx, (const unsigned char *)genSalt4_buf, strlen(genSalt4_buf));
	SHA1_Final(salt_raw, &salt_ctx);
	conv_to_hex(salt_raw, 20, proof_raw);

	unsigned char proof_digest[20];
	// Then generate hashes for the picks
	for( int pick = 0; pick < 10000; pick++ ) {
		double pick2 = pick / 100.0;
		int m = sprintf(&proof_raw[41], "%.2lf", pick2);
		m = strip_trailing_zeroes(41+m, proof_raw);

		SHA1_Init(&salt_ctx);
		SHA1_Update(&salt_ctx, (const unsigned char *)proof_raw, strlen(proof_raw));
		SHA1_Final(proof_digest, &salt_ctx);

		if( 0 == memcmp(proof_digest, search_hash, 20) )
		{
			*found_pick = pick2;
			memcpy(found_hash, proof_digest, 20);
			return 1;
		}
	}
	return 0;
}


static int
hex2data(unsigned char *data, const char *hexstring, unsigned int len)
{
    const char *pos = hexstring;
    char *endptr;
    size_t count = 0;

    if ((hexstring[0] == '\0') || (strlen(hexstring) % 2)) {
        //hexstring contains no data
        //or hexstring has an odd length
        return -1;
    }

    for(count = 0; count < len; count++) {
        char buf[5] = {'0', 'x', pos[0], pos[1], 0};
        data[count] = strtol(buf, &endptr, 0);
        pos += 2 * sizeof(char);

        if (endptr[0] != '\0') {
            //non-hexadecimal character encountered
            return -1;
        }
    }

    return 0;
}

int
main( int argc, char **argv )
{
	time_t start_time;
	unsigned char search_hash[20];
	

	if( argc < 3 )
	{
		fprintf(stderr, "Usage: %s <time> <find-seed> [START END]\n", basename(argv[0]));
		exit(2);
	}
	if( sscanf(argv[1], "%lu", &start_time) != 1 )
	{
		fprintf(stderr, "Error: cannot parse time: %s\n", argv[1]);
		exit(3);
	}
	if( hex2data(search_hash, argv[2], 20) )
	{
		fprintf(stderr, "Error: cannot parse find hash: %s\n", argv[2]);
		exit(4);
	}

	// Parse optional start & end
	unsigned genSalt2_start = 1111111;
	unsigned genSalt2_end = 3333333;
	if( argc > 3 )
	{
		if( sscanf(argv[3], "%u", &genSalt2_start) != 1 )
		{
			fprintf(stderr, "Error: cannot parse START");
			exit(5);
		}
		if( genSalt2_start < 1111111 )
		{
			fprintf(stderr, "Error: invalid START, must be >=1111111\n");
			exit(6);
		}
		if( argc > 4 )
		{
			if( sscanf(argv[4], "%u", &genSalt2_end) != 1 )
			{
				fprintf(stderr, "Error: cannot parse END");
				exit(7);
			}
			if( genSalt2_end > 3333333 )
			{
				fprintf(stderr, "Error: invalid END, must be <=3333333\n");
				exit(8);
			}
		}
	}

	char search_hex[41];
	conv_to_hex(search_hash, 20, search_hex);
	search_hex[40] = 0;
	//fprintf(stderr, "Search Hash: %s\n", search_hex);
	//fprintf(stderr, " Start Time: %lu\n", start_time);
	//fprintf(stderr, " Start Seed: %u\n", genSalt2_start);
	//fprintf(stderr, "   End Seed: %u\n", genSalt2_end);

	size_t genSalt = start_time;
	unsigned char found_hash[20];
	double found_pick;
	benchmark_t bench;
	benchmark_start(&bench);
	for( size_t genSalt2 = genSalt2_start; genSalt2 <= genSalt2_end; genSalt2++ )
	{
		if( bruteforcer(genSalt, genSalt2, search_hash, found_hash, &found_pick) )
		{
			char proof_hex[41];
			conv_to_hex(found_hash, 20, proof_hex);
			proof_hex[40] = 0;
			printf("FOUND %s %.2f\n", proof_hex, found_pick);
			exit(0);
		}
		benchmark_tick(&bench);
	}
	exit(1);
}