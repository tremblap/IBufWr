b = Buffer.alloc(s, s.sampleRate);
c = Buffer.alloc(s, s.sampleRate);

//write at single times the speed values from 0 to 1
(
b.zero;
{
	BufWr.ar(
		LFSaw.ar(1,1).range(0,1),
		b.bufnum,
		LFSaw.ar(1,1).range(0,s.sampleRate-1)
	);
	BufRd.ar(
		1,
		b.bufnum,
		LFSaw.ar(1,1).range(0,s.sampleRate-1),
		0,
		1
	);
}.plot(1)
)

//should be the same at this time
(
c.zero;
{
	IBufWr.ar(
		LFSaw.ar(1,1).range(0,1),
		c.bufnum,
		LFSaw.ar(1,1).range(0,s.sampleRate-1)
	);
	BufRd.ar(
		1,
		c.bufnum,
		LFSaw.ar(1,1).range(0,s.sampleRate-1),
		0,
		1
	);
}.plot(1)
)

/// test number 2
//write at 100 times the speed values from 0 to 1, and play at single time
(
b.zero;
{
	BufWr.ar(
		LFSaw.ar(100,1).range(0,1),
		b.bufnum,
		LFSaw.ar(100,1).range(0,s.sampleRate-1)
	);
	BufRd.ar(
		1,
		b.bufnum,
		LFSaw.ar(1,1).range(0,s.sampleRate-1),
		0,
		1
	);
}.plot()
)

//should be the same at this time
(
c.zero;
{
	IBufWr.ar(
		LFSaw.ar(100,1).range(0,1),
		b.bufnum,
		LFSaw.ar(100,1).range(0,s.sampleRate-1)
	);
	BufRd.ar(
		1,
		b.bufnum,
		LFSaw.ar(1,1).range(0,s.sampleRate-1),
		0,
		1
	);
}.plot()
)

/// test#3 - aural
// all looks fine-ish on both sides
(
b.zero;
c.zero;

{
	var source = SinOsc.ar(200,0,0.05);
	var write_index = LFSaw.ar(10,1).range(0,s.sampleRate-1);
	BufWr.ar(
		source,
		b.bufnum,
		write_index
	);
	IBufWr.ar(
		source,
		c.bufnum,
		write_index
	);
	BufRd.ar(
		1,
		[b.bufnum, c.bufnum],
		LFSaw.ar(10,1).range(0,s.sampleRate-1),
		0,
		1
	);
}.play
)

b.plot;c.plot