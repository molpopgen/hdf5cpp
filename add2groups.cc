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
#include <set>
#include <map>

using namespace std;
using namespace H5;

//Can we get data sets?
herr_t
file_info2(hid_t loc_id, const char *name, const H5L_info_t *linfo, void *opdata)
{
    hid_t dset;
    /*
     * Open the group using its name.
     */
    dset = H5Dopen2(loc_id, name, H5P_DEFAULT);
    /*
     * Display group name.
     */
    cout << "Data set name : " << name << endl;
    H5Dclose(dset);
    return 0;
}

//lifted from http://ftp.hdfgroup.org/HDF5/doc/cpplus_RM/h5group_8cpp-example.html
herr_t
file_info(hid_t loc_id, const char *name, const H5L_info_t *linfo, void *opdata)
{
    hid_t group;
    /*
     * Open the group using its name.
     */
    group = H5Gopen2(loc_id, name, H5P_DEFAULT);
    /*
     * Display group name.
     */
    cout << "Group name : " << name << endl;
    herr_t idx = H5Literate(group, H5_INDEX_NAME, H5_ITER_INC, NULL, file_info2, NULL);
    H5Gclose(group);
    return 0;
}


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
  hsize_t chunk_dims[1] = {1000}; //This make a BIG difference in final file size AND run time!  If you are appending to a lot of big data sets, this should be BIG
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

  //Get open groups
  file.getObjIDs(H5F_OBJ_GROUP,2,&objlist[0]);

  //OK, what did that do?
  for( unsigned i = 0 ; i< file.getObjCount() ; ++i )
    {
      cout << objlist[i] << '\n';
    }

  //The below is taken from the readdata.cpp example provided by the HDF5 group

  //So, the below is "fine" in some sense, but how does one get a list of names of data objects?
  //In other words, do we need to know our names ahead of time?  That seems unlikely, but cannot find documentation...
  delete dataset;
  delete dataspace;
  delete memspace;

  dataset = new DataSet( file.openDataSet("Group0/perms") );

  IntType intype = dataset->getIntType();

  dataspace = new DataSpace( dataset->getSpace() );
  
  int rank = dataspace->getSimpleExtentNdims();
  hsize_t dims_out[rank];
  int ndims = dataspace->getSimpleExtentDims( dims_out, NULL);
  
  cout << "rank " << rank << ", dimensions "
       << (unsigned long)(dims_out[0]) << '\n';  

  offset[0]=0;
  vector<unsigned> receiver(dims_out[0]); //allocate memory to receive

  dataspace->selectHyperslab(H5S_SELECT_SET, dims_out, offset );

  memspace = new DataSpace( rank, dims_out );

  memspace->selectHyperslab(H5S_SELECT_SET, dims_out, offset );

  dataset->read( &receiver[0], intype, *memspace, *dataspace );

  std::set<unsigned> what_did_we_get(receiver.begin(),receiver.end());
  for( set<unsigned>::const_iterator i = what_did_we_get.begin() ; 
       i != what_did_we_get.end() ; ++i )
    {
      cout << *i << '\t'
	   << count(receiver.begin(),receiver.end(),*i) << '\n';
    }
  file.close();

  //Ok, read the file and print out the names of groups and names of data sets nested w/in groups
  file.openFile("testAdding.h5",H5F_ACC_RDONLY);
  herr_t idx = H5Literate(file.getId(), H5_INDEX_NAME, H5_ITER_INC, NULL, file_info, NULL);

  Group root = file.openGroup("/");
  cerr << root.getNumObjs() << '\n';

  //Much more "C++" way of doing it...
  for( unsigned i = 0 ; i < root.getNumObjs() ; ++i )
    {
      cerr << "Group " << i << " -> " << root.getObjnameByIdx(i) << '\n';

      Group subgroup = file.openGroup( root.getObjnameByIdx(i) );
      for( unsigned j = 0 ; j < subgroup.getNumObjs() ; ++j )
	{
	  cerr << '\t' << subgroup.getObjnameByIdx(j) << '\n';
	  DataSet dsj = file.openDataSet( string(root.getObjnameByIdx(i) + "/" + subgroup.getObjnameByIdx(j)).c_str() );
	  DataSpace dspj(dsj.getSpace());

	  int rank_j = dspj.getSimpleExtentNdims();
	  hsize_t dims_out[rank_j];
	  int ndims = dspj.getSimpleExtentDims( dims_out, NULL);
  
	  hsize_t offset[1]={0};
	  vector<unsigned> receiver(dims_out[0]); //allocate memory to receive

	  DataSpace memspace_j(rank_j,dims_out );
	  dspj.selectHyperslab(H5S_SELECT_SET, dims_out, offset );
	  //OK, so read it in!
	  IntType intype = dsj.getIntType();
	  dsj.read( &receiver[0], intype, memspace_j, dspj );
	  set<unsigned> temp(receiver.begin(),receiver.end());
	  for( set<unsigned>::iterator i = temp.begin() ;
	       i != temp.end() ; ++i )
	    {
	      cout << "\t\t" << *i << '\t' << count(receiver.begin(),
					  receiver.end(),*i) << '\n';
	    }
	}
    }
  file.close();

  file.openFile("testAdding.h5",H5F_ACC_RDWR);

  //Write a 2D data set by repeated resizing

  //The writing part below does not work...

  hsize_t dim_init[2] = {1,10};
  vector<double> vd(dim_init[1]); //what will NAN/INF do??
  for( unsigned i = 0 ; i < vd.size() ; ++i )
    {
      vd[i]=i;
    }
  /*
    Knowing that 1 dimension is fixed can 
    really speed things up.
  */
  hsize_t maxdims_init[2] = {H5S_UNLIMITED,10}; //ok, this is important!
  /*
    If the maxdims is fixed, chunk dims should be, too, o/w
    you get errors on trying to create data sets
   */
  hsize_t chunk_dims2[2] = {1000,10};
  cparms.setChunk( 2, chunk_dims2 );
  DataSpace dspace(2,dim_init,maxdims_init);
  DataSet dset(file.createDataSet("Group4/perms",
				  PredType::NATIVE_DOUBLE,
				  dspace,
				  cparms));

  dset.write( &*vd.begin(),	
	      PredType::NATIVE_DOUBLE);


  for( unsigned i = 2 ; i <= 10 ; ++i )
    {
      hsize_t dim_i[2] = {i,10};
      hsize_t offset_i[2] = {i-1,0};
      vd = vector<double>(dim_init[1]);
      for(unsigned j=0;j<vd.size();++j)
	{
	  vd[j]=i+j;
	}

      dset.extend( dim_i );
      DataSpace fspace = dset.getSpace();
      DataSpace mspace( 2, dim_init );
      fspace.selectHyperslab( H5S_SELECT_SET, dim_init , offset_i );

      dset.write(&*vd.begin(),
		 PredType::NATIVE_DOUBLE,
		 mspace,fspace);
      
    }
    
  file.close();
  
  //can we read a vertical slice?
  file.openFile("testAdding.h5",H5F_ACC_RDONLY);

  DataSet g4perms = file.openDataSet("Group4/perms");
  DataSpace g4perms_space = g4perms.getSpace();

  hsize_t hslab_dims[2] = {10,1}; //hyperslab dimensions
  hsize_t hslab_offset[2] = {0,4}; //hslab offset starts from "upper left"
  hsize_t slice_dims[2]={10,1};
  g4perms_space.selectHyperslab(H5S_SELECT_SET, hslab_dims, hslab_offset );

  vector<double> slice(10);

  DataSpace slicespace( 2, slice_dims );

  g4perms.read( &slice[0], PredType::NATIVE_DOUBLE, slicespace, g4perms_space );

  for( unsigned i =0;i<slice.size() ;++i)
    {
      cout << slice[i] << '\n';
    }
}

