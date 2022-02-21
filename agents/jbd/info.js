
printf("====> INFO\n");

//include(libdir+"/utils.js");

var hw = jbd.hw;

var info = {};
info.agent_name = "jbd";
info.agent_role = "Battery";
info.agent_description = "JBD BMS Agent";
info.agent_version = "1.1";
info.agent_author = "Stephen P. Shoecraft";
if (hw.model.length) {
	info.device_manufacturer = hw.manufacturer;
	info.device_model = hw.model;
	info.device_mfgdate = hw.mfgdate;
	info.device_version = hw.version;
}
info.configuration = [];
var mysec = config.sections;
var count = mysec.length;
var sections = [];
for(var i=0; i < count; i++) {
	if (mysec[i].flags & CONFIG_FLAG_NOSAVE) continue;
	sections.push(mysec[i]);
}
var conf = {};
var agent_config = JSON.stringify(sections,[ "name", "type", "size", "scope", "values", "labels", "units", "scale", "precision" ],4);
printf("agent_config: %s\n", agent_config);
conf.agent = JSON.parse(agent_config);
info.configuration.push(conf);

j = JSON.stringify(info,null,4);
//printf("info: %s\n", j);

mqtt.pub(agent.mktopic(SOLARD_FUNC_INFO),j,1);
exit(0);
