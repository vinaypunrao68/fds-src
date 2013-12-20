namespace cpp Sm

service sm_service {
	oneway void put_object(1: string obj_id);
	string get_object(1: string obj_id);
}
