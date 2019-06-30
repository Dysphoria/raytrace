#define uint uint32_t
#define SHR(n,x) ( (x) >> (n) )

uint32_t K[64] = {
	0x428a2f98,0x71374491,
	0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,
	0x923f82a4,0xab1c5ed5,0xd807aa98,0x12835b01,
	0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,
	0x9bdc06a7,0xc19bf174,0xe49b69c1,0xefbe4786,
	0xfc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,
	0x5cb0a9dc,0x76f988da,0x983e5152,0xa831c66d,
	0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,
	0x6ca6351,0x14292967,0x27b70a85,0x2e1b2138,
	0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,
	0x81c2c92e,0x92722c85,0xa2bfe8a1,0xa81a664b,
	0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,
	0xf40e3585,0x106aa070,0x19a4c116,0x1e376c08,
	0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,
	0x5b9cca4f,0x682e6ff3,0x748f82ee,0x78a5636f,
	0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,
	0xbef9a3f7,0xc67178f2 };

uint Ch(uint x, uint y, uint z) {
	return (x&y) ^ (~x&z);
}
uint Maj(uint x, uint y, uint z) {
	return (x&y) ^ (x&z) ^ (y&z);
}
uint ROTR(uint n, uint x) {
	return (x >> n) | (x << (32 - n));
}
uint Sigma0256(uint x) {
	return ROTR(2, x) ^ ROTR(13, x) ^ ROTR(22, x);
}
uint Sigma1256(uint x) {
	return ROTR(6, x) ^ ROTR(11, x) ^ ROTR(25, x);
}
uint sigma0256(uint x) {
	return ROTR(7, x) ^ ROTR(18, x) ^ SHR(3, x);
}
uint sigma1256(uint x) {
	return ROTR(17, x) ^ ROTR(19, x) ^ SHR(10, x);
}

void sha(uint8_t* data, int size, uint H[8]) {

	//Initialize H to the first 8 prime fractionals
	H[0] = 0x6a09e667;
	H[1] = 0xbb67ae85;
	H[2] = 0x3c6ef372;
	H[3] = 0xa54ff53a;
	H[4] = 0x510e527f;
	H[5] = 0x9b05688c;
	H[6] = 0x1f83d9ab;
	H[7] = 0x5be0cd19;

	//W is 2048 bits, we will expand 512 bits into 2048 later each run through the loop
	uint W[64];
	//Do SHA256 stuff on 64 byte(512 bit) chunks of data until there are no chunks left
	while (size > 0) {

		//Do 4 chars at a time (32 bits) for 16 runs = 512 bits, setting the data.
		//Fill W with data followed by 0s
		for (int i = 0; i < 16; ++i) {
			W[i] = (*data) << 24;
			data++;
			W[i] |= (*data) << 16;
			data++;
			W[i] |= (*data) << 8;
			data++;
			W[i] |= (*data);
			data++;
		}

		//Subtract 64 from size to update loop
		size -= 64;

		//Extend the first 16 words into the remaining 48 words W[16..63] of the message schedule array (512 bits -> 2048 bits)
		for (int t = 16; t < 64; ++t)
			W[t] = sigma1256(W[t - 2]) + W[t - 7] + sigma0256(W[t - 15]) + W[t - 16];

		//Initialize the working variables (h, g, f, e, d, c, b, a)
		uint J[8];
		for (int i = 0; i < 8; ++i)
			J[i] = H[i];

		//Compression function loop (2048 bits -> 512 bits)
		for (int t = 0; t < 64; ++t) {
			uint T1 = J[7] + Sigma1256(J[4]) + Ch(J[4], J[5], J[6]) + K[t] + W[t];
			uint T2 = Sigma0256(J[0]) + Maj(J[0], J[1], J[2]);
			J[7] = J[6];
			J[6] = J[5];
			J[5] = J[4];
			J[4] = J[3] + T1;
			J[3] = J[2];
			J[2] = J[1];
			J[1] = J[0];
			J[0] = T1 + T2;
		}

		//Add the compressed chunk(512 bits) onto the current hash value, then continue on to the next chunk
		for (int i = 0; i<8; ++i)
			H[i] += J[i];
	}
}

void shaGPU(uint8_t* data, int size, uint H[8]) {




	//Initialize H to the first 8 prime fractionals
	H[0] = 0x6a09e667;
	H[1] = 0xbb67ae85;
	H[2] = 0x3c6ef372;
	H[3] = 0xa54ff53a;
	H[4] = 0x510e527f;
	H[5] = 0x9b05688c;
	H[6] = 0x1f83d9ab;
	H[7] = 0x5be0cd19;

	//W is 2048 bits, we will expand 512 bits into 2048 later each run through the loop
	uint W[64];
	//Do SHA256 stuff on 64 byte(512 bit) chunks of data until there are no chunks left
	while (size > 0) {

		//Do 4 chars at a time (32 bits) for 16 runs = 512 bits, setting the data.
		//Fill W with data followed by 0s
		for (int i = 0; i < 16; ++i) {
			W[i] = (*data) << 24;
			data++;
			W[i] |= (*data) << 16;
			data++;
			W[i] |= (*data) << 8;
			data++;
			W[i] |= (*data);
			data++;
		}

		//Subtract 64 from size to update loop
		size -= 64;

		//Extend the first 16 words into the remaining 48 words W[16..63] of the message schedule array (512 bits -> 2048 bits)
		for (int t = 16; t < 64; ++t)
			W[t] = sigma1256(W[t - 2]) + W[t - 7] + sigma0256(W[t - 15]) + W[t - 16];

		//Initialize the working variables (h, g, f, e, d, c, b, a)
		uint J[8];
		for (int i = 0; i < 8; ++i)
			J[i] = H[i];

		//Compression function loop (2048 bits -> 512 bits)
		for (int t = 0; t < 64; ++t) {
			uint T1 = J[7] + Sigma1256(J[4]) + Ch(J[4], J[5], J[6]) + K[t] + W[t];
			uint T2 = Sigma0256(J[0]) + Maj(J[0], J[1], J[2]);
			J[7] = J[6];
			J[6] = J[5];
			J[5] = J[4];
			J[4] = J[3] + T1;
			J[3] = J[2];
			J[2] = J[1];
			J[1] = J[0];
			J[0] = T1 + T2;
		}

		//Add the compressed chunk(512 bits) onto the current hash value, then continue on to the next chunk
		for (int i = 0; i<8; ++i)
			H[i] += J[i];
	}
}