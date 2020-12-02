cv = """] , [
		a lv2:InputPort ,
			lv2:CVPort ;
		lv2:index {} ;
		lv2:symbol "{}" ;
		lv2:name "{}" ;
		lv2:default 0 ;
		lv2:minimum 0.0 ;
		lv2:maximum 1.0 ;"""

control = """] , [
		a lv2:InputPort ,
			lv2:ControlPort ;
		lv2:index {} ;
		lv2:symbol "{}" ;
		lv2:name "{}" ;
		lv2:default 0 ;
		lv2:minimum 0.0 ;
		lv2:maximum 1.0 ;"""

ins = """] , [
		a lv2:InputPort ,
			lv2:AudioPort ;
		lv2:index {} ;
		lv2:symbol "{}" ;
		lv2:name "{}" ;
		lv2:default 0 ;
		lv2:minimum 0.0 ;
		lv2:maximum 1.0 ;"""

outs = """] , [
		a lv2:OutputPort ,
			lv2:AudioPort ;
		lv2:index {} ;
		lv2:symbol "{}" ;
		lv2:name "{}" ;"""


t = """
		POLYPHONY_PARAM,
		RESONATOR_PARAM,
		FREQUENCY_PARAM,
		STRUCTURE_PARAM,
		BRIGHTNESS_PARAM,
		DAMPING_PARAM,
		POSITION_PARAM,
		BRIGHTNESS_MOD_PARAM,
		FREQUENCY_MOD_PARAM,
		DAMPING_MOD_PARAM,
		STRUCTURE_MOD_PARAM,
		POSITION_MOD_PARAM,
		BRIGHTNESS_MOD_INPUT,
		FREQUENCY_MOD_INPUT,
		DAMPING_MOD_INPUT,
		STRUCTURE_MOD_INPUT,
		POSITION_MOD_INPUT,
		STRUM_INPUT,
		PITCH_INPUT,
		IN_INPUT,
		ODD_OUTPUT,
		EVEN_OUTPUT,
"""

params = [a.strip().strip(',') for a in t.lower().strip().splitlines()]
for i, p in enumerate(params):
    if p.endswith("_param"):
        print(control.format(str(i), p, p.replace("_", " ").title()))
    elif p.endswith("_input"):
        print(cv.format(str(i), p, p.replace("_", " ").title()))
    elif p.endswith("_output"):
        print(outs.format(str(i), p, p.replace("_", " ").title()))

