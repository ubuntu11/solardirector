
user0 = {name : "user0", id : 0};
user1 = {name : "user1", id : 1};
user2 = {name : "user2", id : 2};

const myArray = [user0,user1,user2];

function uniqueIdIsTwo(user) { 
//	print("user: name: " + user.name + ", id: " + user.id);
	return user.id === 2
}

print("two: "+uniqueIdIsTwo(user0));
print("two: "+uniqueIdIsTwo(user2));
const index = myArray.findIndex(uniqueIdIsTwo);
print("index: "+index);
