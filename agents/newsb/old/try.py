
import json;

with open('sma_objects.json') as f:
	data = json.loads(f.read());
#	print("data: ",data);
	line = "{ ";
	for k in data.keys():
		print("  { \"" + k + "\",");
		print("    {");
		for j in data[k]:
			if j != "TagHier" and j != "GroupChange":
				print("      { \"" + j + "\", " + str(data[k][j]) + " },");
		print("    },");
		for j in data[k]:
			if j == "TagHier":
				s = "    { "
    				for i in data[k][j]: 
					s += str(i) + ","
				s += " },"
				print(s)

		print("  },");
	f.close();
