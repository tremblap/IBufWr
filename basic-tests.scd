b = Buffer.alloc(s, s.sampleRate);
c = Buffer.alloc(s, s.sampleRate);

//write at single times the speed values from 0 to 1
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

/// test#3 - aural difference between BufWr and IBufWr
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

// test #4multichannel buffers tests
d = Buffer.alloc(s, s.sampleRate, 4);

//write at single times the speed values from 0 to 1, and play at single time
(
d.zero;
{
	IBufWr.ar(
		LFSaw.ar([1,1.11,1.22,1.33],1).range(0,1),
		d.bufnum,
		LFSaw.ar(1,1).range(0,s.sampleRate-1)
	);
	BufRd.ar(
		4,
		d.bufnum,
		LFSaw.ar(1,1).range(0,s.sampleRate-1),
		0,
		1
	);
}.plot(1)
)

//write at single times the speed values from 0 to 1 at four different index speeds (does not behave how I thought it would)
(
d.zero;
{
	IBufWr.ar(
		LFSaw.ar(1,1).range(0,1),
		d.bufnum,
		LFSaw.ar([1,1.11,1.22,1.33],1).range(0,s.sampleRate-1)
	);
	BufRd.ar(
		4,
		d.bufnum,
		LFSaw.ar(1,1).range(0,s.sampleRate-1),
		0,
		1
	);
}.plot(1)
)


// test #5 - similar but writing at 4 diff rates, all faster
(
d.zero;
{
	IBufWr.ar(
		LFSaw.ar([10,11.1,12.2,13.3],1).range(0,1),
		d.bufnum,
		LFSaw.ar(10,1).range(0,s.sampleRate-1)
	);
	BufRd.ar(
		4,
		d.bufnum,
		LFSaw.ar(1,1).range(0,s.sampleRate-1),
		0,
		1
	);
}.plot(1)
)

//test #6 - trying arrays of buffers
// this should work
(
b.zero;
c.zero;
{
	IBufWr.ar(
		LFSaw.ar(100,1).range(0,1),
		[b.bufnum,c.bufnum],
		LFSaw.ar(100,1).range(0,s.sampleRate-1)
	);
}.play
)
// works writing both
b.plot;c.plot

//does not work and should not work
(
b.zero;
c.zero;
{
	IBufWr.ar(
		LFSaw.ar([100,200],1).range(0,1),
		[b.bufnum, c.bufnum],
		LFSaw.ar(100,1).range(0,s.sampleRate-1)
	);
}.play
)

// test #7 - too many inputs for number of buff channel in
(
d.zero;
{
	IBufWr.ar(
		LFSaw.ar([1,2,3,4,5],1).range(0,1),
		d.bufnum,
		LFSaw.ar(10,1).range(0,s.sampleRate-1)
	);
	BufRd.ar(
		4,
		d.bufnum,
		LFSaw.ar(1,1).range(0,s.sampleRate-1),
		0,
		1
	);
}.plot(1)
)

// test #8 - writing a value at a given index
(
b.zero;
{
	// var head = Phasor.ar(0, 1 , 0, s.sampleRate);
	var head = LFSaw.ar(1,1).range(0,s.sampleRate-1);
	IBufWr.ar(
		head < 4,
		b.bufnum,
		head
	);
}.plot()
)
b.getn(0,10, {|msg| msg.postln});