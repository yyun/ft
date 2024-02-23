// Copyright [2020-present] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <gtest/gtest.h>

#include <arrow/io/file.h>
#include <parquet/stream_writer.h>

#include <iostream>
#include <string>
#include <vector>

struct Article {
  std::string name;
  float price;
  int quantity;
};

std::vector<Article> get_articles() {
  std::vector<Article> articles{Article{"南昌好景色", 35.0f, 20}, Article{"武汉好风景", 24.0f, 30}, Article{"北京王府井", 50.0f, 10}};
  return std::move(articles);
}

TEST(Test_hdf1, Test) {
  std::shared_ptr<arrow::io::FileOutputStream> out_file;
  PARQUET_ASSIGN_OR_THROW(out_file, arrow::io::FileOutputStream::Open("test.parquet"));

  parquet::WriterProperties::Builder builder;

  parquet::schema::NodeVector fields;

  fields.push_back(
      parquet::schema::PrimitiveNode::Make("name", parquet::Repetition::OPTIONAL, parquet::Type::BYTE_ARRAY, parquet::ConvertedType::UTF8));

  fields.push_back(
      parquet::schema::PrimitiveNode::Make("price", parquet::Repetition::REQUIRED, parquet::Type::FLOAT, parquet::ConvertedType::NONE, -1));

  fields.push_back(
      parquet::schema::PrimitiveNode::Make("quantity", parquet::Repetition::REQUIRED, parquet::Type::INT32, parquet::ConvertedType::INT_32, -1));

  std::shared_ptr<parquet::schema::GroupNode> schema =
      std::static_pointer_cast<parquet::schema::GroupNode>(parquet::schema::GroupNode::Make("schema", parquet::Repetition::REQUIRED, fields));

  parquet::StreamWriter os{parquet::ParquetFileWriter::Open(out_file, schema, builder.build())};

  for (const auto& a : get_articles()) {
    os << a.name << a.price << a.quantity << parquet::EndRow;
  }
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
