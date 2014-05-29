/*
  We want to make groups and then append to them later as we get more data

  Then, read through the file and get the data back into STL containers

  This file contains questions, comments, etc., on quirks that I notice as I go along
 */

#include <H5cpp.h>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <vector>
using namespace std;
using namespace H5;

int main( int argc, char ** argv )
{
  H5File file("testAdding.h5",H5F_ACC_TRUNC);

  //ok -- let's make some blank groups
  for( unsigned i = 0 ; i < 5 ; ++i )
    {
      ostringstream o;
      o << "Group" << i;
      Group group(file.createGroup(o.str().c_str()));
    }

  file.close();

  //reopen for reading and writing
  file.openFile("testAdding.h5",H5F_ACC_RDWR);

  //Group g = file.openGroup("Group0");

  vector<unsigned> foo(100,1);
  hsize_t dim[1];
  dim[0]=foo.size();

  DSetCreatPropList cparms;
  hsize_t chunk_dims[1] = {50};
  hsize_t maxdims[1] = {H5S_UNLIMITED}; //ok, this is important!
  cparms.setChunk( 1, chunk_dims );
  cparms.setDeflate( 6 ); //compression level makes a big differences in large files!  Default is 0 = uncompressed.

  DataSpace * dataspace = new DataSpace(1,dim, maxdims);
  
  DataSet * dataset = new DataSet(file.createDataSet("Group3/perms",
                                                      PredType::NATIVE_INT,
                                                      *dataspace,
                                                      cparms));
  dataset->write( &*foo.begin(),PredType::NATIVE_INT);

  vector<unsigned> foo2(1,101);

  hsize_t size[1];
  size[0]=foo2.size();
  hsize_t offset[1];
  offset[0]=100;
  cerr << size[0] << '\n';

  dim[0] += foo2.size();
  cerr << dim[0] << '\n';
  dataset->extend( dim );

  DataSpace *filespace = new DataSpace( dataset->getSpace() );
  filespace->selectHyperslab( H5S_SELECT_SET, size, offset);
  DataSpace *memspace = new DataSpace( 1, size, NULL );

  dataset->write( &*foo2.begin(),PredType::NATIVE_INT,*memspace,*filespace);

  //Now, add something before Group3/perms
  delete dataset;
  delete dataspace;
  dim[0] = 500;
  foo=vector<unsigned>(500,numeric_limits<unsigned>::max());

  dataspace = new DataSpace(1,dim, maxdims);

  dataset = new DataSet(file.createDataSet("Group0/perms",
					   PredType::NATIVE_UINT, //NOTE: if we don't use UINT, we get overflow and -1 gets written to file!
					   *dataspace,
					   cparms));
 
  dataset->write( &*foo.begin(),PredType::NATIVE_UINT);  //UINT!!

  //Now, let's add 10^6 more elements to this data set
  foo2 = vector<unsigned>(1000000,2);
  offset[0]=dim[0];
  dim[0] += foo2.size();
  size[0] = foo2.size();
  dataset->extend( dim );

  *filespace = DataSpace( dataset->getSpace() ); //good, operator= is defined. Hooray for OOP
  filespace->selectHyperslab( H5S_SELECT_SET, size, offset );
  *memspace = DataSpace(1,size,NULL);

  dataset->write( &*foo2.begin(),PredType::NATIVE_INT,*memspace,*filespace);

  file.close();

  //OK, now, can we read through the groups, data sets w/in groups, and their sizes?

  //reopen for reading and writing
  file.openFile("testAdding.h5",H5F_ACC_RDONLY);

  //Curious--getObjCount() only returns count non-empty things.
  cerr << "There are " << file.getObjCount() << " things in this file\n";

  //let's get the group names
  hid_t objlist[file.getObjCount()];

  //Get open data sets
  file.getObjIDs(H5F_OBJ_GROUP,2,&objlist[0]);

  //OK, what did that do?
  for( unsigned i = 0 ; i< file.getObjCount() ; ++i )
    {
      cout << objlist[i] << '\n';
    }
}

