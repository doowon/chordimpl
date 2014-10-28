//GMP Test Code

#include <stdio.h>
#include <gmp.h>

int main() {

	long i = 1000;
	 printf ("GMP .....  Library: %-12s  Header: %d.%d.%d\n",
          gmp_version, __GNU_MP_VERSION, __GNU_MP_VERSION_MINOR,
          __GNU_MP_VERSION_PATCHLEVEL);
	while (1) {
		usleep(500 * 1000);
		mpz_t z;
		mpz_init(z);
		unsigned char maxValue[20] = {[0 ... 19] = 0xFF};
	mpz_t max; mpz_init(max);
		// mpz_set_ui(z, i++);
		mpz_import(max, 20, 1, sizeof(maxValue[0]), 1, 0, maxValue);
		printf ("i: %lu _mp_size = %d, _mp_alloc = %d\n", i, z->_mp_size, z->_mp_alloc);
	 	// mpz_clear(z);
	}
}