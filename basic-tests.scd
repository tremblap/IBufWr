b = Buffer.alloc(s, s.sampleRate);
c = Buffer.alloc(s, s.sampleRate);

//write at single times the speed values from 0 to 1 // no interp
(
c.zero;
{
	IBufWr.ar(
		Phasor.ar(0, 1, 0, s.sampleRate),
		c.bufnum,
		Phasor.ar(0, 1 , 0, s.sampleRate),
		0,
		0
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
//write at 10 times the speed values from 0 to 1, and play at single time
(
b.zero;
{
	IBufWr.ar(
		Phasor.ar(0, 10, 0, s.sampleRate),
		b.bufnum,
		Phasor.ar(0, 10, 0, s.sampleRate),
		0,// non-interp
		0
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
		1,
		0
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

b.plot;
c.plot

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
		1,//interpolating
		0
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

//write at single times, same values, non interpolating
(
d.zero;
{
	IBufWr.ar(
		Phasor.ar(0, [1000,2000,3000,4000], 0, s.sampleRate),
		d.bufnum,
		Phasor.ar(0, 10, 0, s.sampleRate),
		0,//interpolating
		0
	);
	BufRd.ar(
		4,
		d.bufnum,
		Phasor.ar(0, 1 , 0, s.sampleRate),
		0,
		1
	);
}.plot(0.01)
)


//write at single times the speed values from 0 to 1 at four different index speeds (as it doesn't upscale the phasors, it overwrites like crazy
(
d.zero;
{
	IBufWr.ar(
		Phasor.ar(0, 1.dup(4) , 0, s.sampleRate),
		d.bufnum,
		Phasor.ar(0, [1,1.11,1.22,1.33] , 0, s.sampleRate),
		0, //non interpolating
		0
	);
	BufRd.ar(
		4,
		d.bufnum,
		Phasor.ar(0, 1 , 0, s.sampleRate),
		0,
		1
	);
}.plot(0.01)
)


// test #5 - similar but writing at 4 diff rates, all faster
(
d.zero;
{
	IBufWr.ar(
		Phasor.ar(0, [1,2,3,4], 0, s.sampleRate),
		d.bufnum,
		Phasor.ar(0, 10, 0, s.sampleRate),
		0, //no interp
		0
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
		0
	);
}.asBuffer
)
// works writing both
b.plot;
c.plot

//does not work and should not work
(
b.zero;
c.zero;
{
	IBufWr.ar(
		LFSaw.ar([100,200],1).range(0,1),
		[b.bufnum, c.bufnum],
		Phasor.ar(0, 100 , 0, s.sampleRate),
		1,
		0.5
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
		1,
		0.5
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
		0
	);
}.plot()
)
b.getn(0,10, {|msg| msg.postln});

// testing feedback
(
b.zero;
x = { arg speed = 1, feed = 0, interp = 1;
	var head = Phasor.ar(0, speed, 0, s.sampleRate);
	var sig = BufRd.ar(1,b,head,0,0);
	var trig = Impulse.ar(1.25);
	var sig2 = SinOsc.ar(TIRand.ar(60,72,trig).midicps,mul: Decay.ar(trig,mul: 0.1));
	IBufWr.ar(sig2,b,head,interp,feed);
	Out.ar(0,[sig2,sig]);
}.play
)

x.set(\speed, 0.5)
x.set(\speed, 2)
x.set(\speed, 1)
x.set(\speed, 21.11)

x.set(\feed, 0.8)
x.set(\feed, 0)

x.set(\interp, 0)
x.set(\interp, 1)

//aliases fun
x.set(\interp, 0, \feed, 0.9, \speed, 21.11)
//sorted if interp is on

// TODO benchmarking against BufWr

(
SynthDef(\testBufWr,{
	var source = SinOsc.ar(200,0,0.0001);
	var write_index = Phasor.ar(0, 1 , 0, s.sampleRate);
	IBufWr.ar(
		source,
		b.bufnum,
		write_index,
		0,
		0
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
