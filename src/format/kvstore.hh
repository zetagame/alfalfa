#ifndef KVSTORE_HH
#define KVSTORE_HH

#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>
#include <unordered_map>

#include "optional.hh"

using namespace std;

template<class KeyType, class ValueType>
class KVStore
{
private:
  std::string filename_;
  std::unordered_map<KeyType, ValueType> hash_map_;
  void load( ifstream & fin );

public:
  typedef typename std::unordered_map<KeyType, ValueType>::iterator iterator;
  typedef typename std::unordered_map<KeyType, ValueType>::iterator const_iterator;

  KVStore( const std::string & filename );

  Optional<ValueType> get( const KeyType & key );
  bool insert( KeyType key, ValueType value );
  bool update( KeyType key, ValueType value );
  void update_or_insert( KeyType key, ValueType value );
  bool erase( KeyType & key );
  bool has_key( const KeyType & key ) const;

  void save();

  iterator begin() { return hash_map_.begin(); }
  const_iterator begin() const { return hash_map_.begin(); }
  const_iterator cbegin() const { return hash_map_.cbegin(); }

  iterator end() { return hash_map_.end(); }
  const_iterator end() const { return hash_map_.end(); }
  const_iterator cend() const { return hash_map_.end(); }
};

template<class KeyType, class ValueType>
KVStore<KeyType, ValueType>::KVStore( const std::string & filename )
  : filename_( filename ),
    hash_map_()
{
  std::ifstream fin( filename.c_str() );

  if ( fin.good() ) {
    load( fin );
  }
}

template<class KeyType, class ValueType>
Optional<ValueType> KVStore<KeyType, ValueType>::get( const KeyType & key )
{
  auto entry_it = hash_map_.find( key );

  if ( entry_it != hash_map_.end() ) {
    return make_optional<ValueType>( true, entry_it->second );
  }
  else {
    return Optional<ValueType>();
  }
}

template<class KeyType, class ValueType>
bool KVStore<KeyType, ValueType>::insert( KeyType key, ValueType value )
{
  return hash_map_.emplace( make_pair( key, value ) ).second;
}

template<class KeyType, class ValueType>
bool KVStore<KeyType, ValueType>::update( KeyType key, ValueType value )
{
  auto entry_it = hash_map_.find( key );

  if ( entry_it == hash_map_.end() ) {
    return false;
  }

  entry_it->second = value;
  return true;
}

template<class KeyType, class ValueType>
void KVStore<KeyType, ValueType>::update_or_insert( KeyType key, ValueType value )
{
  hash_map_[key] = value;
}

template<class KeyType, class ValueType>
bool KVStore<KeyType, ValueType>::erase( KeyType & key )
{
  auto entry_it = hash_map_.find( key );

  if ( entry_it == hash_map_.end() ) {
    return false;
  }

  hash_map_.erase( entry_it );
  return true;
}

template<class KeyType, class ValueType>
bool KVStore<KeyType, ValueType>::has_key( const KeyType & key ) const
{
  return hash_map_.find( key ) != hash_map_.end();
}

template<class KeyType, class ValueType>
void KVStore<KeyType, ValueType>::load( ifstream & fin )
{
  hash_map_.clear();

  std::string line;

  while ( std::getline( fin, line ) ) {
    std::istringstream iss( line );

    KeyType key;
    ValueType value;

    if( not ( iss >> key >> value ) ) {
      break;
    }

    insert( key, value );
  }
}

template<class KeyType, class ValueType>
void KVStore<KeyType, ValueType>::save()
{
  ofstream fout( filename_ );

  for ( const auto & entry_it : hash_map_ ) {
    fout << entry_it.first << " " << entry_it.second << endl;
  }

  fout.close();
}

#endif /* KVSTORE_HH */