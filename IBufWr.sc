IBufWr : UGen {
	*ar { arg inputArray, bufnum=0, phase=0.0, interp = 1, feedback = 0.0;
		^this.multiNewList(['audio', bufnum, phase, interp, feedback] ++ inputArray.asArray)
	}
	*kr { arg inputArray, bufnum=0, phase=0.0, interp = 1, feedback = 0.0;
		^this.multiNewList(['control', bufnum, phase, interp, feedback] ++ inputArray.asArray)
	}
	checkInputs {
		if (rate == 'audio' and: {inputs.at(1).rate != 'audio'}, {
			^("phase input is not audio rate: " + inputs.at(1) + inputs.at(1).rate);
		});
		^this.checkValidInputs
	}
}
