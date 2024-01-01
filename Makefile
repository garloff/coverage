CC = gcc
CFLAGS = -g -O2 -Wall $(EXTRA_CFLAGS)
OPTFLAGS = -O3 -Wall -march=native $(EXTRA_CFLAGS)

TARGETS = dist comb chains chains2

default: $(TARGETS)

%: %.o
	$(CC) $(EXTRA_LDFLAGS) -o $@ $^

chains2: chains2.o
	$(CC) $(EXTRA_LDFLAGS) -o $@ $^ -lm

chains2_o: chains2_o.o
	$(CC) $(EXTRA_LDFLAGS) -o $@ $^ -lm

chains2_pg: chains2_pg.o
	$(CC) $(EXTRA_LDFLAGS) -o $@ $^ -lm -pg

dist: dist.o frandom.o
	$(CC) $(EXTRA_LDFLAGS) -o $@ $^

dist_g: dist_g.o frandom_g.o
	$(CC) -g -o $@ $^

dist_o: dist_o.o frandom_o.o
	$(CC) -o $@ $^
	
%.o: %.c
	$(CC) $(CFLAGS) -c $<

%_g.o: %.c
	$(CC) -g -Wall -o $@ -c $<

%_o.o: %.c
	$(CC) $(OPTFLAGS) -o $@ -c $<

%_pg.o: %.c
	$(CC) $(CFLAGS) -pg -o $@ -c $<

clean:
	rm -f $(TARGETS) dist_g dist_o *.o
