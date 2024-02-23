// Copyright [2020-present] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <gtest/gtest.h>

#include "H5Ipublic.h"
#include "hdf5.h"
#include "hdf5_hl.h"
#include <stdlib.h>
 
/*-------------------------------------------------------------------------
 * Table API example
 *
 * H5TBwrite_records
 *
 *-------------------------------------------------------------------------
 */

#define NFIELDS        (hsize_t)5
#define NRECORDS       (hsize_t)8
#define NRECORDS_WRITE (hsize_t)2
#define TABLE_NAME     "table"

struct StepDataInfo
{
    std::string Symbol;
    char Type;
    int64_t RecID;
    double Price;
    int64_t Volume;
    char OrderType;
    int64_t UNIX;
    char OrderCode;
    int64_t RecNO;
    int64_t OrderID;
    int64_t BuyRecID;
    int64_t SellRecID;
    char BuySellFlag;
};



TEST(Test_hdf1, Test) {

          // hid_t是HDF5对象id通用数据类型，每个id标志一个HDF5对象
    hid_t file_id;
    // herr_t是HDF5报错和状态的通用数据类型
    herr_t status;

    // 打开刚建立的HDF文件并关闭
    // 文件id = H5Fopen(const char *文件名, 
    //                  unsigned 读写flags,
    //                    - H5F_ACC_RDWR可读写
    //                    - H5F_ACC_RDONLY只读 
    //                  hid_t 访问性质)
    file_id = H5Fopen("/data/yfxu/20210909/data/output/600000_sort.h5", H5F_ACC_RDONLY, H5P_DEFAULT);
    
    status = H5Lget_info(file_id, "df", NULL, H5P_DEFAULT);
	printf("df: ");
	if (status == 0) 
		printf("The group exists.\n");
	else
		printf("The group either does NOT exist or some other error occurred.\n");
 
 
     hid_t dataset_id;    // 数据集本身的id
	// // dataset_id = H5Dopen(group位置id,
	// //                 const char *name, 数据集名
	// //                    数据集访问性质)
	dataset_id = H5Dopen(file_id, "/df/block2_values", H5P_DEFAULT);

    // 查看数据集dataset中数据的类型以及元数据的字节大小
	hid_t datatype = H5Dget_type(dataset_id);
    int len = H5Tget_size(datatype);
    std::cout << len<<std::endl;

    hid_t dataspace = H5Dget_space(dataset_id);

    hid_t groupid = H5Gopen2(file_id, "df",H5P_DEFAULT); // 打开文件
    int ndims,row;

    int herr=H5LTget_dataset_ndims(groupid,"block2_values",&ndims); // 获得维度
    hsize_t dims[ndims];
    size_t nsz;
    herr=H5LTget_dataset_info(groupid,"block2_values",dims,NULL,&nsz);
    if(herr<0)
    {
        H5Fclose(file_id);
        return ;
    }

    if(ndims == 1)
    {
        row = dims[0];
    }

    int size = nsz; // 单个字符串长度
    char *checkedTime = new char[row*size]; // 分配数据总长度空间
    herr= H5LTread_dataset_string(groupid,"block2_values",checkedTime); // 读取char*数据
    if(herr<0)
    {
        H5Fclose(file_id);
        return ;
    } 

    hsize_t dims1[2];
    hsize_t dimsa[2];
	ndims= H5Sget_simple_extent_dims(dataspace, dims1, dimsa);

    int rows = dims1[0] ;

    
	hid_t dtype = H5Tcopy(H5T_C_S1);

	// long long data_mem [rows][8];
    char data_mem[5][100][100];

    // H5T_NATIVE_DOUBLE
        status = H5Dread(dataset_id, dtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, data_mem);

        for (int i = 0; i < rows; i++) {
        printf("%s ,%s", data_mem[i],data_mem[i]);
        printf("\n");
        }
     
    status = H5Dclose(dataset_id);
    status = H5Fclose(file_id);
 
  
	// delete[] data_mem;
}




// TEST(Test_hdf, Test) {
// //  //! 打开文件和数据集
// // const H5std_string FILE_NAME("/data/yfxu/20210909/data/output/600000_sort.h5");
// // const H5std_string DATASET_NAME("df");
 
//  typedef struct Particle {
//         char   name[16];
//         int    lati;
//         int    longi;
//         float  pressure;
//         double temperature;
//     } Particle;

//     Particle dst_buf[NRECORDS];

//     /* Calculate the size and the offsets of our struct members in memory */
//     size_t dst_size            = sizeof(Particle);
//     size_t dst_offset[NFIELDS] = {HOFFSET(Particle, name), HOFFSET(Particle, lati), HOFFSET(Particle, longi),
//                                   HOFFSET(Particle, pressure), HOFFSET(Particle, temperature)};

//     Particle p                  = {"zero", 0, 0, 0.0F, 0.0};
//     size_t   dst_sizes[NFIELDS] = {sizeof(p.name), sizeof(p.lati), sizeof(p.longi), sizeof(p.pressure),
//                                  sizeof(p.temperature)};

//     /* Define field information */
//     const char *field_names[NFIELDS] = {"Name", "Latitude", "Longitude", "Pressure", "Temperature"};
//     /* Fill value particle */
//     Particle fill_data[1] = {{"no data", -1, -1, -99.0F, -99.0}};
//     hid_t    field_type[NFIELDS];
//     hid_t    string_type;
//     hid_t    file_id;
//     hsize_t  chunk_size = 10;
//     hsize_t  start;    /* Record to start reading/writing */
//     hsize_t  nrecords; /* Number of records to read/write */
//     int      i;

//     /* Define 2 new particles to write */
//     Particle particle_in[NRECORDS_WRITE] = {{"zero", 0, 0, 0.0F, 0.0}, {"one", 10, 10, 1.0F, 10.0}};

//     /* Initialize the field field_type */
//     string_type = H5Tcopy(H5T_C_S1);
//     H5Tset_size(string_type, 16);
//     field_type[0] = string_type;
//     field_type[1] = H5T_NATIVE_INT;
//     field_type[2] = H5T_NATIVE_INT;
//     field_type[3] = H5T_NATIVE_FLOAT;
//     field_type[4] = H5T_NATIVE_DOUBLE;

//     /* Create a new file using default properties. */
//     file_id = H5Fcreate("/data/yfxu/ex_table_03.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

//     /* Make the table */
//     H5TBmake_table("Table Title", file_id, TABLE_NAME, NFIELDS, NRECORDS, dst_size, field_names, dst_offset,
//                    field_type, chunk_size, fill_data, 0, /* no compression */
//                    NULL);                                /* no data written */

//     /* Overwrite 2 records starting at record 0 */
//     start    = 0;
//     nrecords = NRECORDS_WRITE;
//     H5TBwrite_records(file_id, TABLE_NAME, start, nrecords, dst_size, dst_offset, dst_sizes, particle_in);

//     /* read the table */
//     H5TBread_table(file_id, TABLE_NAME, dst_size, dst_offset, dst_sizes, dst_buf);

//     /* print it by rows */
//     for (i = 0; i < NRECORDS; i++) {
//         printf("%-5s %-5d %-5d %-5f %-5f", dst_buf[i].name, dst_buf[i].lati, dst_buf[i].longi,
//                dst_buf[i].pressure, dst_buf[i].temperature);
//         printf("\n");
//     }

//     /* close type */
//     H5Tclose(string_type);

//     /* close the file */
//     H5Fclose(file_id);

// }
