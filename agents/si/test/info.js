
printf("====> INFO\n");

var info = {};
info.agent_name = "si";
info.agent_role = "Inverter";
info.agent_description = "Sunny Island Agent";
info.agent_version = "1.2";
info.agent_author = "Stephen P. Shoecraft";
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
conf.agent = JSON.parse(agent_config);

if (typeof(smanet) != "undefined") {
	var smanet_config = JSON.strinfify(smanet,null,4);
	printf("smanet: %s\n", smanet_config);
	conf.smanet = JSON.parse(smanet_config);
}
info.configuration.push(conf);

j = JSON.stringify(info,null,4);
printf("j: %s\n", j);

mqtt.pub(SOLARD_TOPIC_ROOT+"/"+agent.name+"/"+SOLARD_FUNC_INFO,j,1);
exit(0);
