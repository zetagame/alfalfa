#include "encoder.hh"
#include "uncompressed_chunk.hh"
#include "frame.hh"
#include "bool_encoder.hh"

using namespace std;

Encoder::Encoder( const uint16_t width, const uint16_t height, const Chunk & key_frame )
  : width_( width ), height_( height ),
    state_( KeyFrame( UncompressedChunk( key_frame, width_, height_ ),
		      width_, height_ ).header(),
	    width_, height_ )
{}

vector< uint8_t > Encoder::encode_frame( const Chunk & frame )
{
  /* parse uncompressed data chunk */
  UncompressedChunk uncompressed_chunk( frame, width_, height_ );

  if ( uncompressed_chunk.key_frame() ) {
    /* parse keyframe header */
    KeyFrame myframe( uncompressed_chunk, width_, height_ );

    /* reset persistent decoder state to default values */
    state_ = DecoderState( myframe.header(), width_, height_ );

    /* calculate new probability tables. replace persistent copy if prescribed in header */
    ProbabilityTables frame_probability_tables( state_.probability_tables );
    frame_probability_tables.coeff_prob_update( myframe.header() );
    if ( myframe.header().refresh_entropy_probs ) {
      state_.probability_tables = frame_probability_tables;
    }

    /* decode the frame (and update the persistent segmentation map) */
    myframe.parse_macroblock_headers_and_update_segmentation_map( state_.segmentation_map, frame_probability_tables );
    myframe.parse_tokens( state_.quantizer_filter_adjustments, frame_probability_tables );

    return myframe.serialize_first_partition();
  } else {
    /* parse interframe header */
    InterFrame myframe( uncompressed_chunk, width_, height_ );

    /* update adjustments to quantizer and in-loop deblocking filter */
    state_.quantizer_filter_adjustments.update( myframe.header() );

    /* update probability tables. replace persistent copy if prescribed in header */
    ProbabilityTables frame_probability_tables( state_.probability_tables );
    frame_probability_tables.update( myframe.header() );
    if ( myframe.header().refresh_entropy_probs ) {
      state_.probability_tables = frame_probability_tables;
    }

    /* decode the frame (and update the persistent segmentation map) */
    myframe.parse_macroblock_headers_and_update_segmentation_map( state_.segmentation_map, frame_probability_tables );
    myframe.parse_tokens( state_.quantizer_filter_adjustments, frame_probability_tables );
  }

  return vector< uint8_t >();
}

static void encode( BoolEncoder & encoder, const Flag & flag )
{
  encoder.put( flag );
}

template <class T>
static void encode( BoolEncoder & encoder, const Optional<T> & obj )
{
  if ( obj.initialized() ) {
    encode( encoder, obj.get() );
  }
}

template <class T>
static void encode( BoolEncoder & encoder, const Flagged<T> & obj, const Probability probability = 128 )
{
  encoder.put( obj.initialized(), probability );

  if ( obj.initialized() ) {
    encode( encoder, obj.get() );
  }
}

template <unsigned int width>
static void encode( BoolEncoder & encoder, const Unsigned<width> & num )
{
  encoder.put( num & (1 << (width-1)) );
  encode( encoder, Unsigned<width-1>( num ) );
}

static void encode( BoolEncoder &, const Unsigned<0> & ) {}

template <unsigned int width>
static void encode( BoolEncoder & encoder, const Signed<width> & num )
{
  Unsigned<width> absolute_value = abs( num );
  encode( encoder, absolute_value );
  encoder.put( num < 0 );
}

template <class T, unsigned int len>
static void encode( BoolEncoder & encoder, const Array<T, len> & obj )
{
  for ( unsigned int i = 0; i < len; i++ ) {
    encode( encoder, obj.at( i ) );
  }
}

static void encode( BoolEncoder & encoder, const TokenProbUpdate & tpu,
		    const unsigned int l, const unsigned int k,
		    const unsigned int j, const unsigned int i )
{
  encode( encoder, tpu.coeff_prob, k_coeff_entropy_update_probs.at( i ).at( j ).at( k ).at( l ) );
}

template <class T, unsigned int len, typename... Targs>
static void encode( BoolEncoder & encoder, const Enumerate<T, len> & obj, Targs&&... Fargs )
{
  for ( unsigned int i = 0; i < len; i++ ) {
    encode( encoder, obj.at( i ), i, forward<Targs>( Fargs )... );
  }
}

static void encode( BoolEncoder & encoder, const QuantIndices & h )
{
  encode( encoder, h.y_ac_qi );
  encode( encoder, h.y_dc );
  encode( encoder, h.y2_dc );
  encode( encoder, h.y2_ac );
  encode( encoder, h.uv_dc );
  encode( encoder, h.uv_ac );
}

static void encode( BoolEncoder & encoder, const ModeRefLFDeltaUpdate & h )
{
  encode( encoder, h.ref_update );
  encode( encoder, h.mode_update );
}

static void encode( BoolEncoder & encoder, const SegmentFeatureData & h )
{
  encode( encoder, h.segment_feature_mode );
  encode( encoder, h.quantizer_update );
  encode( encoder, h.loop_filter_update );
}

static void encode( BoolEncoder & encoder, const UpdateSegmentation & h )
{
  encode( encoder, h.update_mb_segmentation_map );
  encode( encoder, h.segment_feature_data );
  encode( encoder, h.mb_segmentation_map );
}

static void encode( BoolEncoder & encoder, const KeyFrameHeader & header )
{
  encode( encoder, header.color_space );
  encode( encoder, header.clamping_type );
  encode( encoder, header.update_segmentation );
  encode( encoder, header.filter_type );
  encode( encoder, header.loop_filter_level );
  encode( encoder, header.sharpness_level );
  encode( encoder, header.mode_lf_adjustments );
  encode( encoder, header.log2_number_of_dct_partitions );
  encode( encoder, header.quant_indices );
  encode( encoder, header.refresh_entropy_probs );
  encode( encoder, header.token_prob_update );
  encode( encoder, header.prob_skip_false );
}

template <class FrameHeaderType, class MacroblockType>
vector< uint8_t > Frame< FrameHeaderType, MacroblockType >::serialize_first_partition( void ) const
{
  BoolEncoder partition;

  /* encode partition header */
  encode( partition, header() );

  /* encode the macroblock headers */


  return partition.finish();
}