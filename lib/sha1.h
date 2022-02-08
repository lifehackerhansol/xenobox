#include <stdint.h>

struct sha1_ctxt {
	union {
		uint8_t	b8[20];
		uint32_t	b32[5];
	} h;
	union {
		uint8_t	b8[8];
		uint64_t	b64[1];
	} c;
	union {
		uint8_t	b8[64];
		uint32_t	b32[16];
	} m;
	uint8_t	count;
};

void sha1_init(struct sha1_ctxt *ctxt);
void sha1_loop(struct sha1_ctxt *ctxt,const uint8_t *input,size_t len);
void sha1_result(struct sha1_ctxt *ctxt,uint8_t *digest);
