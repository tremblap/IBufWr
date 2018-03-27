b = Buffer.alloc(s, s.sampleRate);
c = Buffer.alloc(s, s.sampleRate);

//write at single times the speed values from 0 to 1
(
c.zero;
{
	IBufWr.ar(
		LFSaw.ar(1,1).range(0,1),
		c.bufnum,
		Phasor.ar(0, 1 , 0, s.sampleRate),
		0,
		0.1
	);
	BufRd.ar(
		1,
		c.bufnum,
		Phasor.ar(0, 1 , 0, s.sampleRate),
		0,
		1
	);
}.plot(129/44100)
)
c.getn(0,10, {|msg| msg.postln});

/// test number 2
//write at 100 times the speed values from 0 to 1, and play at single time
(
b.zero;
{
	IBufWr.ar(
		Phasor.ar(0, 1 , 0, s.sampleRate),
		b.bufnum,
		Phasor.ar(0, 1 , 0, s.sampleRate),
		0,
		0.1
	);
	BufRd.ar(
		1,
		b.bufnum,
		Phasor.ar(0, 1 , 0, s.sampleRate),
		0,
		1
	);
}.plot(129/44100)
)
b.getn(0,129, {|msg| msg.postln});

/// test#3 - aural difference between BufWr and IBufWr
// all looks fine-ish on both sides
(
b.zero;
c.zero;

{
	var source = SinOsc.ar(200,0,0.05);
	var write_index = Phasor.ar(0, 10 , 0, s.sampleRate);
	BufWr.ar(
		source,
		b.bufnum,
		write_index
	);
	IBufWr.ar(
		source,
		c.bufnum,
		write_index,
		0,
		0.98
	);
	BufRd.ar(
		1,
		[b.bufnum, c.bufnum],
		write_index,
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
		Phasor.ar(0, 1 , 0, s.sampleRate),
		0,
		0.1
	);
	BufRd.ar(
		4,
		d.bufnum,
		Phasor.ar(0, 1 , 0, s.sampleRate),
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
		Phasor.ar(0, 1 , 0, s.sampleRate),
		d.bufnum,
		Phasor.ar(0, [1,1.11,1.22,1.33] , 0, s.sampleRate),
		0,
		0.1
	);
	BufRd.ar(
		4,
		d.bufnum,
		Phasor.ar(0, 1 , 0, s.sampleRate),
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
		Phasor.ar(0, [1,1.1,2.2,3.3], 0, s.sampleRate),
		d.bufnum,
		Phasor.ar(0, 10, 0, s.sampleRate),
		0,
		0.1
	);
	BufRd.ar(
		4,
		d.bufnum,
		Phasor.ar(0, 1 , 0, s.sampleRate),
		0,
		1
	);
}.plot(129/s.sampleRate)
)
d.getn(0,50, {|msg| msg.postln});

//test #6 - trying arrays of buffers
// this should work
(
b.zero;
c.zero;
{
	IBufWr.ar(
		LFSaw.ar(100,1).range(0,1),
		[b.bufnum,c.bufnum],
		Phasor.ar(0, 100 , 0, s.sampleRate),
		0,
		0.1
	);
}.plot
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
		Phasor.ar(0, 100 , 0, s.sampleRate),
		0,
		0.1
	);
}.plot
)

// test #7 - too many inputs for number of buff channel in
(
d.zero;
{
	IBufWr.ar(
		Phasor.ar(0, [1,2,3,4,5] , 0, s.sampleRate),
		d.bufnum,
		Phasor.ar(0, 10 , 0, s.sampleRate),
		0,
		0.1
	);
	BufRd.ar(
		4,
		d.bufnum,
		Phasor.ar(0, 1 , 0, s.sampleRate),
		0,
		1
	);
}.plot(1)
)

// test #8 - writing a value at a given index
(
b.zero;
{
	var head = Phasor.ar(0, 1 , 0, s.sampleRate);
	IBufWr.ar(
		head < 4,
		b.bufnum,
		head,
		0,
		0.1
	);
}.plot()
)
b.getn(0,10, {|msg| msg.postln});

//benchmarking against BufWr

(
SynthDef(\testBufWr,{
	var source = SinOsc.ar(200,0,0.0001);
	var write_index = Phasor.ar(0, 1 , 0, s.sampleRate);
	IBufWr.ar(
		source,
		b.bufnum,
		write_index,
		0,
		0.1
	);
	Out.ar(0,BufRd.ar(
		1,
		b.bufnum,
		write_index,
		0,
		1
	));
}).send
)

b.zero
500.do({Synth(\testBufWr)})