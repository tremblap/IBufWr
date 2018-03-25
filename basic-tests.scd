b = Buffer.alloc(s, s.sampleRate);

d = Buffer.alloc(s, s.sampleRate, 4);


(
b.zero;
{
	IBufWr.ar(
		DC.ar(1),
		b.bufnum,
		Phasor.ar(0,1,0,s.sampleRate);
	);
}.plot(1E-4)
)
b.getn(0,10, {|msg| msg.postln});


(
d.zero;
{
	IBufWr.ar(
		DC.ar([1,-1,0.1,-0.1]),
		d.bufnum,
		Phasor.ar(0,1,0,s.sampleRate);
	);
}.plot(1)
)
d.getn(0,10, {|msg| msg.postln});