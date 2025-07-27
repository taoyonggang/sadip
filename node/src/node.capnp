@0x8e739ece57a1e2f5;  # 这是一个随机生成的64位标识符，您可能需要更改它

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("cn::seisys::capnp");

struct OctetSeq @0xf537f49f830d2d41 {
  data @0 :Data;
}

enum ArchType @0xa8e1d3c7d2e0f6b5 {
  archTypeUnknown @0;
  amd64 @1;
  x86 @2;
  arm64v8 @3;
}

enum OSType @0xc1e2d3f4a5b6c7d8 {
  osTypeNown @0;
  windowa @1;
  linux @2;
}

enum CmdType @0xd9e8f7c6b5a4d3c2 {
  cmdTypeUnknown @0;
  reboot @1;
  restart @2;
  ls @3;
  mkdir @4;
  del @5;
  deldir @6;
  sendFile @7;
  addApp @8;
  removeApp @9;
  startApp @10;
  stopApp @11;
  restartApp @12;
  lsApp @13;
  addPlugin @14;
  removePlugin @15;
  startPlugin @16;
  stopPlugin @17;
  restartPlugin @18;
  lsPlugin @19;
  getS3File @20;
  putS3File @21;
  lsS3Dir @22;
  lsS3Buckets @23;
  copyS3File @24;
  delS3File @25;
  zip @26;
  dzip @27;
  addS3Watch @28;
  rmS3Watch @29;
  lsS3Watch @30;
  startTraceLog @31;
  stopTraceLog @32;
  lsTraceLog @33;
  startExec @34;
  stopExec @35;
  lsExec @36;
  rmExec @37;
  getExecResult @38;
  mkFile @39;
  cpFile @40;
  podmanCmd @41;
}

enum NodeState @0xb7a6c5d4e3f2a1b0 {
  nodeStateUnknown @0;
  online @1;
  offline @2;
  overload @3;
}

enum PodmanCmd @0xa1b2c3d4e5f6a7b8 {
  listContainers @0;
  createContainer @1;
  startContainer @2;
  stopContainer @3;
  removeContainer @4;
  inspectContainer @5;
  restartContainer @6;
  pauseContainer @7;
  unpauseContainer @8;
  execInContainer @9;
  listImages @10;
  pullImage @11;
  removeImage @12;
  inspectImage @13;
  tagImage @14;
  listNetworks @15;
  createNetwork @16;
  removeNetwork @17;
  inspectNetwork @18;
  listVolumes @19;
  createVolume @20;
  removeVolume @21;
  inspectVolume @22;
  listStacks @23;
  deployStack @24;
  removeStack @25;
  inspectStack @26;
  updateStack @27;
  startStack @28;
  stopStack @29;
  restartStack @30;
  pauseStack @31;
  unpauseStack @32;
  scaleStack @33;
  getStackLogs @34;
  listRegistries @35;
  addRegistry @36;
  removeRegistry @37;
  loginRegistry @38;
}

enum CommStatus @0xc7d6e5f4a3b2c1d0 {
  statusUnknown @0;
  doing @1;
  success @2;
  invalid @3;
  other @4;
}

struct File @0xd1c2b3a4f5e6d7c8 {
  filePathName @0 :Text;
  isDir @1 :Bool;
  fileSize @2 :Int64;
  timestamp @3 :Int64;
}

struct Ping @0xe2d3c4b5a6f7e8d9 {
  toNodeId @0 :Text;
  srcNodeId @1 :Text;
  nodeAppVer @2 :Text;
  nodeCfgVer @3 :Text;
  state @4 :NodeState;
  createdAt @5 :Int64;
  desc @6 :Text;
}

struct Cpu @0xf3e4d5c6b7a8f9e0 {
  vendor @0 :Text;
  model @1 :Text;
  physicalCores @2 :Int32;
  logicalCores @3 :Int32;
  maxFrequency @4 :Int64;
  regularFrequency @5 :Int64;
  minFrequency @6 :Int64;
  currentFrequency @7 :List(Int64);
  cacheSize @8 :Int64;
  cpuUsage @9 :Float64;
}

struct Gpu @0xa4b5c6d7e8f9a0b1 {
  vendor @0 :Text;
  model @1 :Text;
  driverVersion @2 :Text;
  memory @3 :Float64;
  frequency @4 :Int64;
}

struct Ram @0xb5c6d7e8f9a0b1c2 {
  vendor @0 :Text;
  model @1 :Text;
  name @2 :Text;
  serialNumber @3 :Text;
  size @4 :Int64;
  free @5 :Int64;
  available @6 :Int64;
}

struct Disk @0xc6d7e8f9a0b1c2d3 {
  vendor @0 :Text;
  model @1 :Text;
  serialNumber @2 :Text;
  size @3 :Int64;
}

struct MainBoard @0xd7e8f9a0b1c2d3e4 {
  vendor @0 :Text;
  name @1 :Text;
  version @2 :Text;
  serialNumber @3 :Text;
}

struct ProcessTime @0xe8f9a0b1c2d3e4f5 {
  userTime @0 :Float32;
  kernelTime @1 :Float32;
  childrenUsertime @2 :Float32;
  childrenKernelTime @3 :Float32;
}

struct Process @0xf9a0b1c2d3e4f5a6 {
  name @0 :Text;
  workPath @1 :Text;
  pid @2 :Int32;
  args @3 :Text;
  cpuUsage @4 :Float64;
  memUsage @5 :Float64;
  startTime @6 :Int64;
  stopTime @7 :Int64;
  rebootCount @8 :Int32;
  priority @9 :Int32;
  env @10 :List(EnvEntry);

  struct EnvEntry {
    key @0 :Text;
    value @1 :Text;
  }
}

struct Network @0xa0b1c2d3e4f5a6b7 {
  name @0 :Text;
  ipv4s @1 :Text;
  ipv6s @2 :Text;
  broadband @3 :Int64;
  broadbandUsing @4 :Int64;
}

struct OSInfo @0xb1c2d3e4f5a6b7c8 {
  operatingSystem @0 :Text;
  shortName @1 :Text;
  version @2 :Text;
  kernel @3 :Text;
  architecture @4 :Text;
  endianess @5 :Text;
}

struct MachineState @0xc2d3e4f5a6b7c8d9 {
  toNodeId @0 :Text;
  srcNodeId @1 :Text;
  cpus @2 :List(Cpu);
  gpus @3 :List(Gpu);
  rams @4 :Ram;
  disks @5 :List(Disk);
  networks @6 :List(Network);
  mainBoardInfos @7 :MainBoard;
  osInfos @8 :OSInfo;
  processes @9 :List(Process);
}

struct Node @0xd3e4f5a6b7c8d9e0 {
  uuid @0 :Text;
  toNodeId @1 :Text;
  toNodeName @2 :Text;
  srcNodeId @3 :Text;
  toGroup @4 :Text;
  arch @5 :ArchType;
  os @6 :OSType;
  uri @7 :Text;
  toConfigPath @8 :Text;
  srcConfigPath @9 :Text;
  domain @10 :UInt32;
  config @11 :Data;
  configHash @12 :Text;
  configSize @13 :UInt32;
  state @14 :NodeState;
  createdAt @15 :Int64;
  updatedAt @16 :Int64;
}

struct NodeReply @0xe4f5a6b7c8d9e0f1 {
  uuid @0 :Text;
  toNodeId @1 :Text;
  toNodeName @2 :Text;
  srcNodeId @3 :Text;
  toGroup @4 :Text;
  replyUuid @5 :Text;
  status @6 :CommStatus;
  desc @7 :Text;
  descStr @8 :Text;
  createdAt @9 :Int64;
  updatedAt @10 :Int64;
}

struct NodeCmd @0xf5a6b7c8d9e0f1a2 {
  uuid @0 :Text;
  srcNodeId @1 :Text;
  srcNodeName @2 :Text;
  toNodeId @3 :Text;
  toGroup @4 :Text;
  cmdType @5 :CmdType;
  podmanCmd @6 :PodmanCmd;
  cmd @7 :Text;
  paras @8 :Text;
  paras1 @9 :Text;
  paras2 @10 :Text;
  createdAt @11 :Int64;
  updatedAt @12 :Int64;
}

struct NodeCmdReply @0xa6b7c8d9e0f1a2b3 {
  uuid @0 :Text;
  srcNodeId @1 :Text;
  srcNodeName @2 :Text;
  toNodeId @3 :Text;
  toNodeName @4 :Text;
  cmdReplyUuid @5 :Text;
  cmdType @6 :CmdType;
  status @7 :CommStatus;
  desc @8 :Text;
  descStr @9 :Text;
  result @10 :List(File);
  createdAt @11 :Int64;
  updatedAt @12 :Int64;
}