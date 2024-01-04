CC = gcc
CFLAGS = -g -O2 -Wall $(EXTRA_CFLAGS)
OPTFLAGS = -O3 -Wall -march=native $(EXTRA_CFLAGS)

TARGETS = dist comb chains chains2 chains2_o chains2_o_as chains2_o_p0 chains3 chains3_o chains3_om chains3_o_p0

default: $(TARGETS)

%: %.o
	$(CC) $(EXTRA_LDFLAGS) -o $@ $^

chains3: chains3.o
	$(CC) $(EXTRA_LDFLAGS) -o $@ $^ -lm

chains3_o: chains3_o.o
	$(CC) $(EXTRA_LDFLAGS) -o $@ $^ -lm

chains3_om: chains3_om.o
	$(CC) $(EXTRA_LDFLAGS) -o $@ $^ -lm

chains3_o_p0: chains3_o_p0.o
	$(CC) $(EXTRA_LDFLAGS) -o $@ $^ -lm

chains3_pg: chains3_pg.o
	$(CC) $(EXTRA_LDFLAGS) -o $@ $^ -lm -pg

chains2: chains2.o
	$(CC) $(EXTRA_LDFLAGS) -o $@ $^ -lm

chains2_o: chains2_o.o
	$(CC) $(EXTRA_LDFLAGS) -o $@ $^ -lm

chains2_o_as: chains2_o_as.o
	$(CC) $(EXTRA_LDFLAGS) -o $@ $^ -lm

chains2_o_p0: chains2_o_p0.o
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

%_om.o: %.c
	$(CC) $(OPTFLAGS) -DSAVE_SCALE -o $@ -c $<

%_o_as.o: %.c
	$(CC) $(OPTFLAGS) -DAVOID_SUBNORMAL -o $@ -c $<

%_o_p0.o: %.c
	$(CC) $(OPTFLAGS) -DPREFETCH=0 -o $@ -c $<

%_pg.o: %.c
	$(CC) $(CFLAGS) -pg -o $@ -c $<

clean:
	rm -f $(TARGETS) dist_g dist_o *.o
