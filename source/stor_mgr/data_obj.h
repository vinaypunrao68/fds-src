/*------------------------------------------------------------------------------------------
 *
 *
 *              This file is a property of FORMATION DATA SYSTEMS Inc. 
 *              
 * 
 *-----------------------------------------------------------------------------------------*/

typedef  fds_data_hash_t int[4]; /* 128 bit unique hash */

typedef fds_data_obj_t {
   fds_data_hash_t  data_hash; 
   fds_uint8_t      conflict_id;
   fds_uint8_t      data_obj_size;
   fds_uint8_t      archive;   /* 00=no archive, 01=local archive, 02=site2 archive */
   fds_uint8_t      state;
};


typedef fds_data_object_pkg {
   fds_data_obj_t     data_obj;
   fds_data_obj_id_t  data_obj_id;
   fds_uint32_t       virtual_vol_id;
};
