/* a very simplified version of a Forth text interpreter

   Input: A sequence of the following:
   
   1) '\n' (line feed) followed by a character identifying a wordlist
      followed by a name: define the name in the wordlist
   2) '\t' (tab) followed by a sequence of characters:
      set the search order; the bottom of the search order is first,
      the top last
   3) ' ' (space) followed by a name:
      look up the name in the search order; there may be names that are
       not in the search order.

   Names do not contain characters <= ' ', and these characters are
   also not used for identifying wordlists.

   To verify that these things work, every defined word gets a serial
   number (starting with 1) and a hash is computed across all found
   words  */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>

#define  HASHTABLE 1
#define  USE_GPERF 1


#ifdef USE_GPERF
#include "gperf-hash.c"
#endif

#include <limits.h>

/* a wordlist is organized as a linked list, so you can reorganize it
   as you see fit */
typedef struct list_entry {
  struct list_entry *next;
  unsigned char *name;
  size_t name_len;
  unsigned long serialno;
} list_entry;


typedef struct {
    int size;
    list_entry  **table; 
} hashtable_t ;


/* each wordlist is identified by a character, used as indexes into
   the following array */
#ifdef HASHTABLE
hashtable_t *wordlists[256];
#else
list_entry *wordlists[256];
#endif


/* the search order is a sequence of characters, starting with the
   bottom, represented with the pointer and length here */
unsigned char *order;
size_t order_len;







hashtable_t *ht_create( int size ) 
{
    hashtable_t *hashtable = NULL;
    int i;

    if( size < 1 ) return NULL;

    /* Allocate the table itself. */
    if( ( hashtable = malloc( sizeof( hashtable_t ) ) ) == NULL ) {
        return NULL;
    }

    /* Allocate pointers to the head nodes. */
    if( ( hashtable->table = malloc( sizeof( list_entry * ) * size ) ) == NULL ) {
        return NULL;
    }
    for( i = 0; i < size; i+=10 ) {
        hashtable->table[i] = NULL;
		hashtable->table[i+1] = NULL;
		hashtable->table[i+2] = NULL;
		hashtable->table[i+3] = NULL;
		hashtable->table[i+4] = NULL;
		hashtable->table[i+5] = NULL;
		hashtable->table[i+6] = NULL;
		hashtable->table[i+7] = NULL;
		hashtable->table[i+8] = NULL;
		hashtable->table[i+9] = NULL;
    }

    hashtable->size = size;

    return hashtable;   
}


/* Hash a string for a particular hash table. */
int ht_hash( hashtable_t *hashtable, unsigned char *key, size_t key_len ) {

#ifdef USE_GPERF
    unsigned long int hashval = hash((const char*) key, key_len);
#else
    unsigned long int hashval=0 ;
    int i = 0;

    /* Convert our string to an integer */
	if( hashval < ULONG_MAX && i < key_len ){
		hashval = hashval << 8;
		hashval += key[ i ];
		i++;
		while( hashval < ULONG_MAX && i < key_len ) {
			hashval = hashval << 8;
			hashval += key[ i ];
			i++;
		}
	}
#endif

    return hashval % hashtable->size;
}



/* Insert a key-value pair into a hash table. */
void ht_set( hashtable_t *hashtable, unsigned char *key, size_t key_len, int serialno ) 
{
    int bin = ht_hash( hashtable, key, key_len );

    list_entry *new = malloc(sizeof(list_entry));

    new->name= key;
    new->name_len= key_len;
    new->serialno= serialno;
    new->next= hashtable->table[ bin ];

    hashtable->table[ bin ]= new;
}

list_entry *ht_get( hashtable_t *hashtable, unsigned char *key, size_t key_len ) 
{
    int bin = ht_hash( hashtable, key, key_len );

    list_entry *pair = hashtable->table[ bin ];
	if(pair != NULL && pair->name != NULL && 
			   (key_len != pair->name_len || memcmp(key,pair->name,key_len)!=0) ){
		pair=pair->next;
		while( pair != NULL && pair->name != NULL && 
			   (key_len != pair->name_len || memcmp(key,pair->name,key_len)!=0) ) 
		{
			pair = pair->next;
		}
	}
    return pair;
}



/* insert name starting at s+1 (and ending at the next char <=' ')
   into the wordlist given at *s, associate serialno with it; return
   the first character after the name */
unsigned char *create(unsigned char *s, unsigned long serialno) {
  unsigned char w=*s++;
  size_t i;
  for (i=0; s[i]>' '; i++)
    ;


#ifdef HASHTABLE

  hashtable_t *hashtable= wordlists[w];
  
  if( hashtable == NULL )
  {
    hashtable= ht_create( 4000 );
    wordlists[w]= hashtable;
  }

  ht_set( hashtable, s, i, serialno );

#else

  list_entry *new = malloc(sizeof(list_entry));

  new->next = wordlists[w];
  new->name = s;
  new->name_len = i;
  new->serialno = serialno;
  wordlists[w] = new;

#endif


  return s+i;
}

/* set the search order to the one specified by starting at s, and
   ending at the first character <=' '; return the first character
   after the search order */
unsigned char *set_order(unsigned char *s) {
  order = s;
  for (order_len=0; s[order_len]>' '; order_len++)
    ;
  return order+order_len;
}

/* look up the name starting at s with length s_len in the wordlist
   wl; if successfull, store the serialno of the word in foundp,
   otherwise 0 */
void search_wordlist(unsigned char *s, size_t s_len, list_entry *wl, unsigned long *foundp) {
  for (; wl != NULL; wl = wl->next) {
    if (s_len == wl->name_len && memcmp(s,wl->name,s_len)==0) {
      *foundp = wl->serialno;
      return;
    }
  }
  *foundp = 0;
  return;
}

/* look up the name starting at s and ending at the next char <=' ' in
   the search order, storing the serialno of the word in foundp if
   successful, otherwise 0; return the first character after the
   name */
unsigned char *find(unsigned char *s, unsigned long *foundp) {
  size_t i;
  signed long j;
  for (i=0; s[i]>' '; i++)
    ;
#ifdef HASHTABLE
	for (j=order_len-1; j>=0; j--) {
    
		hashtable_t *ht= wordlists[order[j]];

		if( ht == NULL )
			continue;

		list_entry *l= ht_get( ht , s, i );
		if( l != NULL )
		{
			*foundp= l->serialno;
			return s+i;
		}
	}
#else
    for (j=order_len-1; j>=0; j--) {
		search_wordlist(s,i,wordlists[order[j]],foundp);
		if (*foundp != 0)
		  return s+i;
	}
#endif
  *foundp = 0;
  return s+i;
}

/* process the input starting at s and ending at the first '\0' */
unsigned long process(unsigned char *s) {
  unsigned long hash = 0;
  unsigned long serialno = 1;
  unsigned long found;
  unsigned long k0=0xb64d532aaaaaaad5;
  while (1) {
    switch (*s++) {
    case '\0': return hash;
    case '\n': s=create(s,serialno++); break;
    case '\t': s=set_order(s); break;
    case ' ' : 
      { 
        /* unsigned char *s1=s; */
        s=find(s,&found);
        /* fwrite(s1,1,s-s1,stdout); printf(" = %ld\n",found);} */
        if (found!=0) {
          hash=(hash^found)*k0;
          hash^= (hash>>41);
        }
      }
      break;
    default:
      fprintf(stderr,"invalid input");
      exit(1);
    }
  }
  return 0;
}

int main(int argc, char* argv[]) {
  int fd;
  struct stat buf;
  unsigned char *s;
  if (argc!=2) {
    fprintf(stderr,"Usage: %s <file>\n",argv[0]);
    exit(1);
  }



/*
hashtable_t * hashtable= ht_create( 4000 );
  unsigned char str[]= "esel";
  size_t str_len= 4;
  unsigned char str2[]= "katze";
  size_t str2_len= 5;

  ht_set( hashtable, str, str_len, 23 );
  ht_set( hashtable, str, str_len, 24 );
  ht_set( hashtable, str2, str2_len, 25 );
list_entry *l= ht_get( hashtable, str, str_len );

  if( l )
        printf("%d\n", l->serialno );
  else
        printf("not found in hashtable\n" );

list_entry *l2= ht_get( hashtable, str2, str2_len );

  if( l2 )
        printf("%d\n", l2->serialno );
  else
        printf("not found in hashtable\n" );

*/


  fd=open(argv[1], O_RDONLY);
  if (fd==-1) {
    perror(argv[1]);
    exit(1);
  }
  if (fstat(fd, &buf) == -1) {
    perror(argv[1]);
    exit(1);
  }
  s = mmap(NULL, buf.st_size+1, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
  s[buf.st_size] = '\0';
  printf("%lx\n",process(s));
  return 0;
}


