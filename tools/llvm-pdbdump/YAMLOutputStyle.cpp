//===- YAMLOutputStyle.cpp ------------------------------------ *- C++ --*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "YAMLOutputStyle.h"

#include "PdbYaml.h"
#include "llvm-pdbdump.h"

#include "llvm/DebugInfo/PDB/Raw/InfoStream.h"
#include "llvm/DebugInfo/PDB/Raw/PDBFile.h"
#include "llvm/DebugInfo/PDB/Raw/RawConstants.h"

using namespace llvm;
using namespace llvm::pdb;

YAMLOutputStyle::YAMLOutputStyle(PDBFile &File) : File(File), Out(outs()) {}

Error YAMLOutputStyle::dump() {
  if (opts::pdb2yaml::StreamDirectory || opts::pdb2yaml::PdbStream)
    opts::pdb2yaml::StreamMetadata = true;

  if (auto EC = dumpFileHeaders())
    return EC;

  if (auto EC = dumpStreamMetadata())
    return EC;

  if (auto EC = dumpStreamDirectory())
    return EC;

  if (auto EC = dumpPDBStream())
    return EC;

  flush();
  return Error::success();
}

Error YAMLOutputStyle::dumpFileHeaders() {
  yaml::MsfHeaders Headers;
  Obj.Headers.SuperBlock.NumBlocks = File.getBlockCount();
  Obj.Headers.SuperBlock.BlockMapAddr = File.getBlockMapIndex();
  Obj.Headers.BlockMapOffset = File.getBlockMapOffset();
  Obj.Headers.SuperBlock.BlockSize = File.getBlockSize();
  auto Blocks = File.getDirectoryBlockArray();
  Obj.Headers.DirectoryBlocks.assign(Blocks.begin(), Blocks.end());
  Obj.Headers.NumDirectoryBlocks = File.getNumDirectoryBlocks();
  Obj.Headers.SuperBlock.NumDirectoryBytes = File.getNumDirectoryBytes();
  Obj.Headers.NumStreams =
      opts::pdb2yaml::StreamMetadata ? File.getNumStreams() : 0;
  Obj.Headers.SuperBlock.Unknown0 = File.getUnknown0();
  Obj.Headers.SuperBlock.Unknown1 = File.getUnknown1();
  Obj.Headers.FileSize = File.getFileSize();

  return Error::success();
}

Error YAMLOutputStyle::dumpStreamMetadata() {
  if (!opts::pdb2yaml::StreamMetadata)
    return Error::success();

  Obj.StreamSizes = File.getStreamSizes();
  return Error::success();
}

Error YAMLOutputStyle::dumpStreamDirectory() {
  if (!opts::pdb2yaml::StreamDirectory)
    return Error::success();

  auto StreamMap = File.getStreamMap();
  Obj.StreamMap.emplace();
  for (auto &Stream : StreamMap) {
    pdb::yaml::StreamBlockList BlockList;
    BlockList.Blocks = Stream;
    Obj.StreamMap->push_back(BlockList);
  }

  return Error::success();
}

Error YAMLOutputStyle::dumpPDBStream() {
  if (!opts::pdb2yaml::PdbStream)
    return Error::success();

  auto IS = File.getPDBInfoStream();
  if (!IS)
    return IS.takeError();

  auto &InfoS = IS.get();
  Obj.PdbStream.emplace();
  Obj.PdbStream->Age = InfoS.getAge();
  Obj.PdbStream->Guid = InfoS.getGuid();
  Obj.PdbStream->Signature = InfoS.getSignature();
  Obj.PdbStream->Version = InfoS.getVersion();

  return Error::success();
}

void YAMLOutputStyle::flush() {
  Out << Obj;
  outs().flush();
}
