#include	"../src/CIFS.h"
#include	<iconv.h>
#include	"CIFS.h"

struct Log			*pLog;
iconv_t	iConv;

int
main( int ac, char *av[] )
{
	IdMap_t	*pIM;
	uint16_t	Id;
	int	i;

#define	SIZE	0x10000 //10

	pIM = IdMapCreate( SIZE, Malloc, Free );

	for( i = 0; i < 10; i++ ) {
		Id	= IdMapGet( pIM );
		printf("Id[%u]\n", Id );
	}
	IdMapPut( pIM, 3 );
	printf("put:Id[%u]\n", 3 );
	Id	= IdMapGet( pIM );
	printf("get:Id[%u]\n", Id );

	IdMapPut( pIM, 3 );
	printf("put:Id[%u]\n", 3 );
	IdMapPut( pIM, 4 );
	printf("put:Id[%u]\n", 4 );
	Id	= IdMapGet( pIM );
	printf("get:Id[%u]\n", Id );
	return( 0 );
}
