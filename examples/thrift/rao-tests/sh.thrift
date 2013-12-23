namespace cpp Sh

service sh_service {
	oneway void put_object_done(1: string obj_data)
}
