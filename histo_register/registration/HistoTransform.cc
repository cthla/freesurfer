#include "registration/HistoTransform.h"
#include <sbl/core/Command.h>
#include <sbl/core/PathConfig.h>
#include <sbl/math/MathUtil.h>
#include <sbl/system/FileSystem.h>
#include <sbl/image/ImageUtil.h>
#include <sbl/image/MotionFieldUtil.h>
#include <sbl/image/ImageDraw.h>
using namespace sbl;
namespace hb {


//-------------------------------------------
// HISTO TRANSFORM CLASS
//-------------------------------------------


/// load transformation data generated by the histo registration commands
bool HistoTransform::loadHToB( const String &mrRawPath, const String &mrRegLinPath, 
							   const String &mrRegPath, const String &histoSplitPath, const String &histoRegPath ) {

	// load block offset
	// blockOffset is index of blockface image corresponding to slice #1 (not necessarily the first histology image)
	String fileName = histoRegPath + "/blockOffset.txt";
	File blockOffsetFile( fileName, FILE_READ, FILE_TEXT );
	if (blockOffsetFile.openSuccess()) {
		m_blockOffset = blockOffsetFile.readInt();
	} else {
		warning( "unable to read block offset file: %s", fileName.c_str() );
		return false;
	}
	disp( 1, "blockOffset: %d", m_blockOffset );

	// load histology motion fields
	Array<String> mfFileList = dirFileList( histoRegPath, "flow_", ".mf" );
	if (mfFileList.count() == 0) {
		warning( "unable to find histo motion field files at: %s", histoRegPath.c_str() );
		return false;
	}
	for (int i = 0; i < mfFileList.count(); i++) {
		String fileName = histoRegPath + "/" + mfFileList[ i ];
		aptr<MotionField> mf = loadMotionField( fileName );
		int mfWidth = mf->width(), mfHeight = mf->height();
		m_mfHisto.append( mf.release() );
		int sliceIndex = mfFileList[ i ].rightOfLast( '_' ).leftOfLast( '.' ).toInt();
		m_histoSliceIndex.append( sliceIndex );

		// load the histo image transform
		fileName = histoRegPath + sprintF( "/transform_%d.dat", sliceIndex );
		File transformFile( fileName, FILE_READ, FILE_BINARY );
		if (transformFile.openSuccess()) {
			aptr<ImageTransform> transform( new ImageTransform( transformFile ) );
			m_histoTransform.append( transform.release() );
		} else {
			warning( "unable to load histo transform file: %s", fileName.c_str() );
			return false;
		}

		// load the full-size histo image (after split)
		fileName = histoSplitPath + sprintF( "/%d.png", sliceIndex );
		aptr<ImageGrayU> image = sbl::load<ImageGrayU>( fileName ); // fix(faster): we could get the image dimensions without loading the entire image
		int width = image->width(), height = image->height();
		m_histoShrinkX.append( (double) mfWidth / (double) width );
		m_histoShrinkY.append( (double) mfHeight / (double) height );
	}
	disp( 1, "histo shrink for slice zero: %.2f, %.2f", m_histoShrinkX[ 0 ], m_histoShrinkY[ 0 ] );
	disp( 1, "motion field size: %d x %d", m_mfHisto[ 0 ].width(), m_mfHisto[ 0 ].height() );
	return true;
} 


/// load transformation data generated by the block-to-MR registration commands
bool HistoTransform::loadBToM( const String &mrRawPath, const String &mrRegLinPath, 
							   const String &mrRegPath, const String &histoSplitPath, const String &histoRegPath ) {

	// load linear volume transform
	disp( 1, "loading linear volume transform" );
	String regLinTransformFileName = mrRegLinPath + "/linearTransform.txt";
	File regLinTransformFile( regLinTransformFileName, FILE_READ, FILE_TEXT );
	if (regLinTransformFile.openSuccess()) {
		m_regLinTransform = loadTransform( regLinTransformFile );
	} else {
		warning( "unable to open transform file: %s", regLinTransformFileName.c_str() );
		return false;
	}
	disp( 1, "M/B reg lin scale: %.2f, %.2f, %.2f, offset: %.2f, %.2f, %.2f",
			m_regLinTransform.a.data[ 0 ][ 0 ], m_regLinTransform.a.data[ 1 ][ 1 ], m_regLinTransform.a.data[ 2 ][ 2 ], 
			m_regLinTransform.b.x, m_regLinTransform.b.y, m_regLinTransform.b.z );

	// load non-linear volume transform
	String cfSeqFileName = mrRegPath + "/cfSeq.cfs";
	m_cfSeq = loadCorresSeq( cfSeqFileName );
	if (m_cfSeq.count() == 0) {
		warning( "unable to open corres volume file: %s", cfSeqFileName.c_str() );
		return false;
	}

	// load MR file transform
	disp( 1, "loading MR file transform" );
	String mrTransformFileName = mrRawPath + "/volumeTransform.txt";
	File mrTransformFile( mrTransformFileName, FILE_READ, FILE_TEXT );
	if (mrTransformFile.openSuccess()) {
		m_mrTransform = loadTransform( mrTransformFile );
	} else {
		warning( "unable to open MR transform file: %s", mrTransformFileName.c_str() );
		return false;
	}
	return true;
}


/// compute inverses of histo-to-blockface mappings; this must be called before projectBackward()
void HistoTransform::computeInversesHToB() {
	disp( 1, "computing H to B inverses" );

	// computer inverse of linear and non-linear slice transforms
	for (int i = 0; i < m_mfHisto.count(); i++) {

		// invert motion field
		int width = m_mfHisto[ i ].width(), height = m_mfHisto[ i ].height();
		float uMin = 0, uMean = 0, uMax = 0, vMin = 0, vMean = 0, vMax = 0;
		motionFieldStats( m_mfHisto[ i ], uMin, vMin, uMean, vMean, uMax, vMax );
		disp( 1, "%d of %d, %5.3f / %5.3f / %5.3f, %5.3f / %5.3f / %5.3f", i, m_mfHisto.count(), uMin, uMean, uMax, vMin, vMean, vMax );
		ImageGrayU mask( width, height );
		mask.clear( 255 );
		int sampleStep = 4;
		aptr<MotionField> mfInv = invertMotionField( m_mfHisto[ i ], mask, sampleStep );
		motionFieldStats( *mfInv, uMin, vMin, uMean, vMean, uMax, vMax );
		m_mfHistoInv.append( mfInv.release() );
		disp( 1, "%d of %d, %5.3f / %5.3f / %5.3f, %5.3f / %5.3f / %5.3f", i, m_mfHisto.count(), uMin, uMean, uMax, vMin, vMean, vMax );

		// invert linear transform
		m_histoTransformInv.append( m_histoTransform[ i ].inverse().release() );
	}
	disp( 1, "done computing H to B inverses" );
}


/// compute inverses of blockface-to-MR mappings; this must be called before projectBackward()
void HistoTransform::computeInversesBToM() {
	disp( 1, "computing B to M inverses" );

	// compute inverse of MR file transform
	m_mrTransformInv = m_mrTransform.inverse();

	// compute inverse of linear volume registration transform
	m_regLinTransformInv = m_regLinTransform.inverse();
	disp( 1, "M/B reg lin inverse scale: %.2f, %.2f, %.2f, offset: %.2f, %.2f, %.2f",
			m_regLinTransformInv.a.data[ 0 ][ 0 ], m_regLinTransformInv.a.data[ 1 ][ 1 ], m_regLinTransformInv.a.data[ 2 ][ 2 ], 
			m_regLinTransformInv.b.x, m_regLinTransformInv.b.y, m_regLinTransformInv.b.z );

	// compute inverse of non-linear volume transform
	m_cfSeqInv = invert( m_cfSeq );
	disp( 1, "done computing B to M inverses" );
}


/// project a point from histology coordinates to MR coordinates
Point3 HistoTransform::projectForward( Point3 point, bool useMrTransform, bool smallHistoCoords, bool verbose ) const {
	point = projectHToB( point, smallHistoCoords, verbose );
	if (point.x || point.y || point.z) {
		point = projectBToM( point, useMrTransform, true, verbose );
	}
	return point;
}


/// project a point from MR coordinates to histology coordinates
Point3 HistoTransform::projectBackward( Point3 point, bool useMrTransform, bool smallHistoCoords, bool verbose ) const {
	point = projectMToB( point, useMrTransform, true, verbose );
	if (point.x || point.y || point.z) {
		point = projectBToH( point, smallHistoCoords, verbose );
	}
	return point;
}


/// project a point from histo coordinates to block-face coordinates
Point3 HistoTransform::projectHToB( Point3 point, bool smallHistoCoords, bool verbose ) const {
	if (verbose) disp( 1, "init: %3.1f, %3.1f, %3.1f", point.x, point.y, point.z );

	// get histo index
	int mfIndex = findHistoArrayIndex( sbl::round( point.z ) );
	if (mfIndex == -1) {
		warning( "unable to find flow for histo index: %d", sbl::round( point.z ) );
		point.x = 0;
		point.y = 0;
		point.z = 0; 
		return point;
	}

	// shrink x, y coordinates to block-face size
	if (smallHistoCoords == false) {
		point.x *= m_histoShrinkX[ mfIndex ];
		point.y *= m_histoShrinkY[ mfIndex ];
	}
	if (verbose) disp( 1, "after histo shrink: %3.1f, %3.1f, %3.1f", point.x, point.y, point.z );

	// map according to linear image transform
	double xNew = m_histoTransformInv[ mfIndex ].xTransform( (float) point.x, (float) point.y );
	double yNew = m_histoTransformInv[ mfIndex ].yTransform( (float) point.x, (float) point.y );
	point.x = xNew;
	point.y = yNew;
	if (verbose) disp( 1, "after histo linear: %3.1f, %3.1f, %3.1f", point.x, point.y, point.z );

	// map according to flow
	const MotionField &mf = m_mfHistoInv[ mfIndex ];
	if (mf.inBounds( (float) point.x, (float) point.y ) == false) {
		if (verbose)
			warning( "projected out of motion field: %f, %f (mf size: %d, %d)", point.x, point.y, mf.width(), mf.height() );
		point.x = 0;
		point.y = 0;
		point.z = 0; 
		return point;
	}
	float u = mf.uInterp( (float) point.x, (float) point.y );
	float v = mf.vInterp( (float) point.x, (float) point.y );
	point.x += u;
	point.y += v;
	if (verbose) disp( 1, "after mf: %3.1f, %3.1f, %3.1f", point.x, point.y, point.z );

	// map z coord
	point.z = m_histoSliceIndex[ mfIndex ] + m_blockOffset;
	if (verbose) disp( 1, "after block offset: %3.1f, %3.1f, %3.1f", point.x, point.y, point.z );
	return point;
}


/// project a point from block-face coordinates to MR coordinates
Point3 HistoTransform::projectBToM( Point3 point, bool useMrTransform, bool useNonLinear, bool verbose ) const {

	// apply non-linear volume mapping
	if (useNonLinear) {
		int xInt = round( (float) point.x );
		int yInt = round( (float) point.y );
		int zInt = round( (float) point.z );
		int cfWidth = m_cfSeq[ 0 ].width(), cfHeight = m_cfSeq[ 0 ].height();
		if (xInt < 0 || xInt >= cfWidth || yInt < 0 || yInt >= cfHeight || zInt < 0 || zInt >= m_cfSeq.count()) {
			if (verbose)
				warning( "projected out of corres volume: %d, %d, %d", xInt, yInt, zInt );
			point.x = 0;
			point.y = 0;
			point.z = 0; 
			return point;
		}
		point.x += m_cfSeq[ zInt ].u( xInt, yInt );
		point.y += m_cfSeq[ zInt ].v( xInt, yInt );
		point.z += m_cfSeq[ zInt ].w( xInt, yInt );
		if (verbose) disp( 1, "after reg non-lin: %3.1f, %3.1f, %3.1f", point.x, point.y, point.z );
	}

	// apply linear transform
	point = m_regLinTransform.transform( point );
	if (verbose) disp( 1, "after reg lin: %3.1f, %3.1f, %3.1f", point.x, point.y, point.z );

	// apply MR transform
	if (useMrTransform) {
		point = m_mrTransform.transform( point );
		if (verbose) disp( 1, "after MR: %3.1f, %3.1f, %3.1f", point.x, point.y, point.z );
	}
	return point;
}


/// project a point from MR coordinates to block-face coordinates
Point3 HistoTransform::projectMToB( Point3 point, bool useMrTransform, bool useNonLinear, bool verbose ) const {
	if (verbose) disp( 1, "init: %3.1f, %3.1f, %3.1f", point.x, point.y, point.z );

	// apply MR transform
	if (useMrTransform) {
		point = m_mrTransformInv.transform( point );
		if (verbose) disp( 1, "after MR inv: %3.1f, %3.1f, %3.1f", point.x, point.y, point.z );
	}

	// apply linear transform
	point = m_regLinTransformInv.transform( point );
	if (verbose) disp( 1, "after reg lin: %3.1f, %3.1f, %3.1f", point.x, point.y, point.z );

	// apply non-linear field
	if (useNonLinear) {
		int xInt = round( (float) point.x );
		int yInt = round( (float) point.y );
		int zInt = round( (float) point.z );
		int cfWidth = m_cfSeq[ 0 ].width(), cfHeight = m_cfSeq[ 0 ].height();
		if (xInt < 0 || xInt >= cfWidth || yInt < 0 || yInt >= cfHeight || zInt < 0 || zInt >= m_cfSeq.count()) {
			warning( "projected out of corres volume: %d, %d, %d", xInt, yInt, zInt );
			point.x = 0;
			point.y = 0;
			point.z = 0; 
			return point;
		}
		point.x += m_cfSeqInv[ zInt ].u( xInt, yInt );
		point.y += m_cfSeqInv[ zInt ].v( xInt, yInt );
		point.z += m_cfSeqInv[ zInt ].w( xInt, yInt );
		if (verbose) disp( 1, "after reg non-lin: %3.1f, %3.1f, %3.1f", point.x, point.y, point.z );
	}
	return point;
}


/// project a point from block-face coordinates to MR coordinates
Point3 HistoTransform::projectBToH( Point3 point, bool smallHistoCoords, bool verbose ) const {

	// map z coord
	point.z = point.z - m_blockOffset;
	if (verbose) disp( 1, "after block offset: %3.1f, %3.1f, %3.1f", point.x, point.y, point.z );

	// map according to flow
	int mfIndex = findHistoArrayIndex( sbl::round( point.z ) );
	if (mfIndex == -1) {
		if (verbose) warning( "unable to find flow for histo index: %d", sbl::round( point.z ) );
		point.x = 0;
		point.y = 0;
		point.z = 0; 
		return point;
	}	
	const MotionField &mfInv = m_mfHisto[ mfIndex ];
	if (mfInv.inBounds( (float) point.x, (float) point.y ) == false) {
		if (verbose) warning( "projected out of motion field: %f, %f", point.x, point.y );
		point.x = 0;
		point.y = 0;
		point.z = 0; 
		return point;
	}
	float u = mfInv.uInterp( (float) point.x, (float) point.y );
	float v = mfInv.vInterp( (float) point.x, (float) point.y );
	point.x += u;
	point.y += v;	
	if (verbose) disp( 1, "after mf inv: %3.1f, %3.1f, %3.1f", point.x, point.y, point.z );

	// map according to linear image transform
	double xNew = m_histoTransform[ mfIndex ].xTransform( (float) point.x, (float) point.y );
	double yNew = m_histoTransform[ mfIndex ].yTransform( (float) point.x, (float) point.y );
	point.x = xNew;
	point.y = yNew;
	if (verbose) disp( 1, "after histo linear inv: %3.1f, %3.1f, %3.1f", point.x, point.y, point.z );

	// expand x, y coordinates from block-face size to histo size
	if (smallHistoCoords == false) {
		point.x /= m_histoShrinkX[ mfIndex ];
		point.y /= m_histoShrinkY[ mfIndex ];
	}
	if (verbose) disp( 1, "after histo unshrink: %3.1f, %3.1f, %3.1f", point.x, point.y, point.z );
	return point;
}


// find the index within the motion field array corresponding to the given slice index
int HistoTransform::findHistoArrayIndex( int z ) const {
	int mfIndex = -1;
	for (int i = 0; i < m_histoSliceIndex.length(); i++) {
		if (m_histoSliceIndex[ i ] == z) 
			mfIndex = i;
	}
	return mfIndex;
}


//-------------------------------------------
// COMMANDS
//-------------------------------------------


/// project a set of points from histology coordinates to MR coordinates or vice versa
void projectPoints( Config &conf ) {

	// get command parameters
	String inputFileName = addDataPath( conf.readString( "inputFileName" ) );
	String outputFileName = addDataPath( conf.readString( "outputFileName" ) );
	bool projectForward = conf.readBool( "projectForward", true );
	bool smallHistoCoords = conf.readBool( "smallHistoCoords", false ); // true if using post-hprep coordinates
	String mrRawPath = addDataPath( conf.readString( "mrRawPath", "mri/rawFlash20" ) );
	String mrRegLinPath = addDataPath( conf.readString( "mrRegLinPath", "mri/regLin" ) );
	String mrRegPath = addDataPath( conf.readString( "mrRegPath", "mri/reg" ) );
	String histoSplitPath = addDataPath( conf.readString( "histoSplitPath" "histo/split" ) );
	String histoRegPath = addDataPath( conf.readString( "histoRegPath" "histo/reg" ) );

	// open input file 
	File inputFile( inputFileName, FILE_READ, FILE_TEXT );
	if (inputFile.openSuccess() == false) {
		warning( "error reading input file: %s", inputFileName.c_str() );
		return;
	}

	// open output file
	File outputFile( outputFileName, FILE_WRITE, FILE_TEXT );
	if (outputFile.openSuccess() == false) {
		warning( "error opening output file: %s", outputFileName.c_str() );
		return;
	}

	// load transformation data
	HistoTransform histoTransform;
	if (histoTransform.loadHToB( mrRawPath, mrRegLinPath, mrRegPath, histoSplitPath, histoRegPath ) == false) {
		return; // don't need to print warning; transform load will do that
	}
	if (histoTransform.loadBToM( mrRawPath, mrRegLinPath, mrRegPath, histoSplitPath, histoRegPath ) == false) {
		return; // don't need to print warning; transform load will do that
	}

	// loop over input points, projecting each one
	while (inputFile.endOfFile() == false) {

		// read input point
		String inputLine = inputFile.readLine();
		Array<String> inputSplit = inputLine.split( "," );
		if (inputSplit.count()) {
			if (inputSplit.count() != 3) {
				warning( "invalid line: %s (expect 3 numbers separated by commas)", inputLine.c_str() );
			}
			Point3 point;
			point.x = inputSplit[ 0 ].strip().toFloat();
			point.y = inputSplit[ 1 ].strip().toFloat();
			point.z = inputSplit[ 2 ].strip().toFloat();

			// project histo coord to MR coord
			if (projectForward) {
				point = histoTransform.projectForward( point, true, smallHistoCoords, false );

			// project MR coord to histo coord
			} else {
				point = histoTransform.projectBackward( point, true, smallHistoCoords, false );
			}

			// write output line
			outputFile.writeF( "%f,%f,%f\n", point.x, point.y, point.z );
		}

		// check for user cancel
		if (checkCommandEvents())
			break;
	}
}


/// test histo-to-MR-to-histo coordinate projection using assorted points from the histology slices
void testProjectPoints( Config &conf ) {

	// get command parameters
	String mrRawPath = addDataPath( conf.readString( "mrRawPath", "mri/rawFlash20" ) );
	String mrRegLinPath = addDataPath( conf.readString( "mrRegLinPath", "mri/regLin" ) );
	String mrRegPath = addDataPath( conf.readString( "mrRegPath", "mri/reg" ) );
	String histoSplitPath = addDataPath( conf.readString( "histoSplitPath", "histo/split" ) );
	String histoRegPath = addDataPath( conf.readString( "histoRegPath", "histo/reg" ) );
	String outputPath = addDataPath( conf.readString( "outputPath", "histo/proj" ) );
	int step = conf.readInt( "step", 100 );
	bool verbose = conf.readBool( "verbose", true );

	// load transformation data
	HistoTransform histoTransform;
	if (histoTransform.loadHToB( mrRawPath, mrRegLinPath, mrRegPath, histoSplitPath, histoRegPath ) == false) {
		return; // don't need to print warning; transform load will do that
	}
	if (histoTransform.loadBToM( mrRawPath, mrRegLinPath, mrRegPath, histoSplitPath, histoRegPath ) == false) {
		return; // don't need to print warning; transform load will do that
	}
	histoTransform.computeInversesHToB();
	histoTransform.computeInversesBToM();

	// check for user cancel
	if (checkCommandEvents())
		return;

	// get list of input (post-prep) histo images
	Array<String> fileList = dirFileList( histoSplitPath, "", ".png" );
	if (fileList.count() == 0) {
		warning( "unable to find histo motion field files at: %s", histoSplitPath.c_str() );
		return;
	}

	// make sure output path exists
	createDir( outputPath );

	// loop over histo images
	VectorD allError;
	int badCount = 0;
	for (int i = 0; i < fileList.count(); i++) {

		// load the histo image and compute a mask 
		String fileName = histoSplitPath + "/" + fileList[ i ];
		aptr<ImageGrayU> image = load<ImageGrayU>( fileName );
		int hWidth = image->width(), hHeight = image->height();
		aptr<ImageGrayU> mask = threshold( *image, 254, true );
		int hSliceIndex = fileList[ i ].leftOfLast( '.' ).toInt();

		// we will save diagnostic images for the middle histo image 
		aptr<ImageColorU> histoMarkers;
		if (i == fileList.count() / 2)
			histoMarkers = toColor( *image );

		// loop over image, checking points in mask
		VectorD sliceError;
		for (int y = 0; y < hHeight; y += step) {
			for (int x = 0; x < hWidth; x += step) {
				if (mask->data( x, y )) {
					Point3 point;
					point.x = x;
					point.y = y;
					point.z = hSliceIndex;
					Point3 forPoint = histoTransform.projectForward( point, true, false, verbose );
					if (forPoint.x == 0 && forPoint.y == 0 && forPoint.z == 0) {
						badCount++;
						continue;
					}
					Point3 backPoint = histoTransform.projectBackward( forPoint, true, false, verbose );
					if (backPoint.x == 0 && backPoint.y == 0 && backPoint.z == 0) {
						badCount++;
						continue;
					}
					double xDiff = point.x - backPoint.x;
					double yDiff = point.y - backPoint.y;
					double zDiff = point.z - backPoint.z;
					double err = sqrt( xDiff * xDiff + yDiff * yDiff + zDiff * zDiff );
					sliceError.append( err );
					allError.append( err );
					if (histoMarkers.get()) {
						drawCross( *histoMarkers, x, y, 10, 255, 0, 0, true );
					}
				}
			}
		}
		disp( 1, "slice: %d, error: %f / %f / %f", hSliceIndex, sliceError.min(), sliceError.mean(), sliceError.max() );

		// save diagnostic images
		if (histoMarkers.get()) {
			saveImage( *histoMarkers, outputPath + "/histoMarkers.png" );
		}

		// check for user cancel
		if (checkCommandEvents())
			break;
	}
	disp( 1, "error: %f / %f / %f", allError.min(), allError.mean(), allError.max() );
	disp( 1, "bad count: %d", badCount );
}


/// this command projects an MR volume (e.g. labels) into histology space using nearest neighbor interpolation
void projectVolume( Config &conf ) {

	// get command parameters
	String mrRawPath = addDataPath( conf.readString( "mrRawPath", "mri/rawFlash20" ) );
	String mrRegLinPath = addDataPath( conf.readString( "mrRegLinPath", "mri/regLin" ) );
	String mrRegPath = addDataPath( conf.readString( "mrRegPath", "mri/reg" ) );
	String histoSplitPath = addDataPath( conf.readString( "histoSplitPath", "histo/split" ) );
	String histoRegPath = addDataPath( conf.readString( "histoRegPath", "histo/reg" ) );
	String inputPath = addDataPath( conf.readString( "inputPath", "mri/label" ) );
	String outputPath = addDataPath( conf.readString( "outputPath", "mri/labelProj" ) );

	// load transformation data
	HistoTransform histoTransform;
	if (histoTransform.loadHToB( mrRawPath, mrRegLinPath, mrRegPath, histoSplitPath, histoRegPath ) == false) {
		return; // don't need to print warning; loadTransform will do that
	}
	if (histoTransform.loadBToM( mrRawPath, mrRegLinPath, mrRegPath, histoSplitPath, histoRegPath ) == false) {
		return; // don't need to print warning; transform load will do that
	}
	histoTransform.computeInversesHToB();
	histoTransform.computeInversesBToM();

	// check for user cancel
	if (checkCommandEvents())
		return;
	
	// make sure output path exists
	createDir( outputPath );

	// get list of input (post-prep) histo images
	Array<String> hFileList = dirFileList( histoSplitPath, "", ".png" );
	if (hFileList.count() == 0) {
		warning( "unable to find .png files at: %s", histoSplitPath.c_str() );
		return;
	}

	// load MR volume
	Array<String> mFileList = dirFileList( inputPath, "", ".png" );
	if (mFileList.count() == 0) {
		warning( "unable to find .png files at: %s", inputPath.c_str() );
		return;
	}
	Array<ImageGrayU> mrVolume;
	for (int i = 0; i < mFileList.count(); i++) {
		String fileName = inputPath + "/" + mFileList[ i ];
		aptr<ImageGrayU> mrSlice = load<ImageGrayU>( fileName );		
		mrVolume.append( mrSlice.release() );
	}

	// load first histo image to get dimensions
	int hWidth = 0, hHeight = 0;
	aptr<ImageGrayU> hImage = load<ImageGrayU>( histoSplitPath + "/" + hFileList[ 0 ] );		
	hWidth = hImage->width();
	hHeight = hImage->height();
	hImage.reset();

	// loop over histo images, generating an MR projection image for each one
	for (int i = 0; i < hFileList.count(); i++) {
		int hSliceIndex = hFileList[ i ].leftOfLast( '.' ).toInt();
		aptr<ImageGrayU> projImage( new ImageGrayU( hWidth, hHeight ) );
		projImage->clear( 0 );
		for (int y = 0; y < hHeight; y++) {
			for (int x = 0; x < hWidth; x++) {
				Point3 point;
				point.x = x;
				point.y = y;
				point.z = hSliceIndex;
				bool verbose = false;
				if (x == hWidth / 2 && y == hHeight / 2 && i == hFileList.count() / 2)
					verbose = true;
				Point3 mrPoint = histoTransform.projectForward( point, false, false, verbose );
				if (mrPoint.x || mrPoint.y || mrPoint.z) {
					int xMrInt = sbl::round( mrPoint.x );
					int yMrInt = sbl::round( mrPoint.y );
					int zMrInt = sbl::round( mrPoint.z );
					if (zMrInt >= 0 && zMrInt < mrVolume.count()) {
						ImageGrayU &mrSlice = mrVolume[ zMrInt ];
						if (mrSlice.inBounds( xMrInt, yMrInt )) {
							projImage->data( x, y ) = mrSlice.data( xMrInt, yMrInt );
						}
					}
				}
			}
		}
		saveImage( *projImage, outputPath + "/" + hFileList[ i ] );

		// check for user cancel
		if (checkCommandEvents())
			break;
	}
}


/// project a histology volume to blockface coordinates (save as volume)
void projectHistoToBlockFace( Config &conf ) {

	// get command parameters
	String inputPath = addDataPath( conf.readString( "inputPath", "histo/label" ) );
	String outputPath = addDataPath( conf.readString( "outputPath", "histo/labelProj" ) );
	String mrRawPath = addDataPath( conf.readString( "mrRawPath", "mri/rawFlash20" ) );
	String mrRegLinPath = addDataPath( conf.readString( "mrRegLinPath", "mri/regLin" ) );
	String mrRegPath = addDataPath( conf.readString( "mrRegPath", "mri/reg" ) );
	String histoSplitPath = addDataPath( conf.readString( "histoSplitPath", "histo/split" ) );
	String histoRegPath = addDataPath( conf.readString( "histoRegPath", "histo/reg" ) );
	bool smallHistoCoords = false;

	// load transformation data
	// fix(faster); don't load B to M data (we do it just because we need to know the block-face dims)
	HistoTransform histoTransform;
	if (histoTransform.loadHToB( mrRawPath, mrRegLinPath, mrRegPath, histoSplitPath, histoRegPath ) == false) {
		return; // don't need to print warning; transform load will do that
	}
	if (histoTransform.loadBToM( mrRawPath, mrRegLinPath, mrRegPath, histoSplitPath, histoRegPath ) == false) {
		return; // don't need to print warning; transform load will do that
	}

	// check for user cancel
	if (checkCommandEvents())
		return;
	
	// make sure output path exists
	createDir( outputPath );

	// get block-face dimensions
	int bDepth = histoTransform.blockDepth();
	int bWidth = histoTransform.blockWidth();
	int bHeight = histoTransform.blockHeight();

	// loop over block-face slices
	for (int bIndex = 0; bIndex < bDepth; bIndex++) {

		// create output image in block-face coordinates
		ImageGrayU outputImage( bWidth, bHeight );

		// compute histo index from block index
		Point3 point;
		point.x = bWidth / 2;
		point.y = bHeight / 2;
		point.z = bIndex;
		point = histoTransform.projectBToH( point, smallHistoCoords, false );

		// if histo file exists, use it (otherwise, save black image)
		if (point.x || point.y || point.z) {
			int histoIndex = sbl::round( point.z );
			String hFileName = inputPath + sprintF( "/%d.png", histoIndex );
			if (fileExists( hFileName )) {
			
				// load histo image
				aptr<ImageGrayU> inputImage = load<ImageGrayU>( hFileName );

				// loop over block-face coordinates, obtaining values from histo image
				for (int y = 0; y < bHeight; y++) {
					for (int x = 0; x < bWidth; x++) {
						point.x = x;
						point.y = y;
						point.z = bIndex;
						point = histoTransform.projectBToH( point, smallHistoCoords, false );
						int xHisto = sbl::round( point.x );
						int yHisto = sbl::round( point.y );
						int histoValue = 0;
						if (inputImage->inBounds( xHisto, yHisto ))
							histoValue = inputImage->data( xHisto, yHisto );
						outputImage.data( x, y ) = histoValue;
					}
				}
			} else {
				outputImage.clear( 0 );
			}

		// if no histo image, we will save empty image
		} else {
			outputImage.clear( 0 );
		}

		// save image (block-face coordinates)
		String outputFileName = outputPath + sprintF( "/%d.png", bIndex );
		saveImage( outputImage, outputFileName );

		// check for user cancel
		if (checkCommandEvents())
			break;
	}
}


/// project an MR volume to blockface coordinates (save as volume)
void projectMriToBlockFace( Config &conf ) {

	// get command parameters
	String inputPath = addDataPath( conf.readString( "inputPath", "mri/label" ) );
	String outputPath = addDataPath( conf.readString( "outputPath", "mri/labelProj" ) );
	bool useNonLinear = conf.readBool( "useNonLinear", true );
	String mrRawPath = addDataPath( conf.readString( "mrRawPath", "mri/rawFlash20" ) );
	String mrRegLinPath = addDataPath( conf.readString( "mrRegLinPath", "mri/regLin" ) );
	String mrRegPath = addDataPath( conf.readString( "mrRegPath", "mri/reg" ) );
	String histoSplitPath = addDataPath( conf.readString( "histoSplitPath", "histo/split" ) );
	String histoRegPath = addDataPath( conf.readString( "histoRegPath", "histo/reg" ) );
	bool useMrTransform = false;

	// load transformation data
	HistoTransform histoTransform;
	if (histoTransform.loadBToM( mrRawPath, mrRegLinPath, mrRegPath, histoSplitPath, histoRegPath ) == false) {
		return; // don't need to print warning; transform load will do that
	}
	histoTransform.computeInversesBToM();

	// check for user cancel
	if (checkCommandEvents())
		return;
	
	// make sure output path exists
	createDir( outputPath );

	// load MR volume
	Array<String> mFileList = dirFileList( inputPath, "", ".png" );
	if (mFileList.count() == 0) {
		warning( "unable to find .png files at: %s", inputPath.c_str() );
		return;
	}
	Array<ImageGrayU> mrVolume;
	for (int i = 0; i < mFileList.count(); i++) {
		String fileName = inputPath + "/" + mFileList[ i ];
		aptr<ImageGrayU> mrSlice = load<ImageGrayU>( fileName );		
		mrVolume.append( mrSlice.release() );
	}

	// get block-face dimensions
	int bDepth = histoTransform.blockDepth();
	int bWidth = histoTransform.blockWidth();
	int bHeight = histoTransform.blockHeight();

	// test projection (verbose) on mid point
	Point3 point;
	point.x = bWidth / 2;
	point.y = bHeight / 2;
	point.z = 83; //bDepth / 2;
	disp( 1, "projecting point: %.1f, %.1f, %.1f", point.x, point.y, point.z );
	point = histoTransform.projectBToM( point, useMrTransform, useNonLinear, true );

	// loop over block-face slices
	for (int bIndex = 0; bIndex < bDepth; bIndex++) {

		// create output image in block-face coordinates
		ImageGrayU outputImage( bWidth, bHeight );

		// loop over block-face coordinates, obtaining values from MR volume
		for (int y = 0; y < bHeight; y++) {
			for (int x = 0; x < bWidth; x++) {
				Point3 point;
				point.x = x;
				point.y = y;
				point.z = bIndex;
				point = histoTransform.projectBToM( point, useMrTransform, useNonLinear, false );
				if (point.x || point.y || point.z) {
					int mx = sbl::round( point.x );
					int my = sbl::round( point.y );
					int mz = sbl::round( point.z );
					int mValue = 0;
					if (mz >= 0 && mz < mrVolume.count()) {
						ImageGrayU &mSlice = mrVolume[ mz ];
						if (mSlice.inBounds( mx, my ))
							mValue = mSlice.data( mx, my );
					}
					outputImage.data( x, y ) = mValue;
				}
			}
		}

		// save image (block-face coordinates)
		String outputFileName = outputPath + sprintF( "/%d.png", bIndex );
		saveImage( outputImage, outputFileName );

		// check for user cancel
		if (checkCommandEvents())
			break;
	}
}


//-------------------------------------------
// INIT / CLEAN-UP
//-------------------------------------------


// register commands, etc. defined in this module
void initHistoTransform() {
	registerCommand( "hproj", projectPoints );
	registerCommand( "hprojvol", projectVolume );
	registerCommand( "hprojtest", testProjectPoints );
	registerCommand( "hprojb", projectHistoToBlockFace );
	registerCommand( "mprojb", projectMriToBlockFace );
}


} // end namespace hb
