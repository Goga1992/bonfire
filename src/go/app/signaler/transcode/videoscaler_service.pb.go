// Code generated by protoc-gen-go. DO NOT EDIT.
// versions:
// 	protoc-gen-go v1.28.0
// 	protoc        v3.6.1
// source: videoscaler_service.proto

package transcode

import (
	empty "github.com/golang/protobuf/ptypes/empty"
	protoreflect "google.golang.org/protobuf/reflect/protoreflect"
	protoimpl "google.golang.org/protobuf/runtime/protoimpl"
	reflect "reflect"
	sync "sync"
)

const (
	// Verify that this generated code is sufficiently up-to-date.
	_ = protoimpl.EnforceVersion(20 - protoimpl.MinVersion)
	// Verify that runtime/protoimpl is sufficiently up-to-date.
	_ = protoimpl.EnforceVersion(protoimpl.MaxVersion - 20)
)

type StartVideoSlotRequest struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	SinkHostname string   `protobuf:"bytes,1,opt,name=sink_hostname,json=sinkHostname,proto3" json:"sink_hostname,omitempty"`
	SinkPorts    []uint32 `protobuf:"varint,2,rep,packed,name=sink_ports,json=sinkPorts,proto3" json:"sink_ports,omitempty"` // TODO: port to resolutions
}

func (x *StartVideoSlotRequest) Reset() {
	*x = StartVideoSlotRequest{}
	if protoimpl.UnsafeEnabled {
		mi := &file_videoscaler_service_proto_msgTypes[0]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *StartVideoSlotRequest) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*StartVideoSlotRequest) ProtoMessage() {}

func (x *StartVideoSlotRequest) ProtoReflect() protoreflect.Message {
	mi := &file_videoscaler_service_proto_msgTypes[0]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use StartVideoSlotRequest.ProtoReflect.Descriptor instead.
func (*StartVideoSlotRequest) Descriptor() ([]byte, []int) {
	return file_videoscaler_service_proto_rawDescGZIP(), []int{0}
}

func (x *StartVideoSlotRequest) GetSinkHostname() string {
	if x != nil {
		return x.SinkHostname
	}
	return ""
}

func (x *StartVideoSlotRequest) GetSinkPorts() []uint32 {
	if x != nil {
		return x.SinkPorts
	}
	return nil
}

type VideoSlotInfo struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	Id       string `protobuf:"bytes,1,opt,name=id,proto3" json:"id,omitempty"`
	Hostname string `protobuf:"bytes,2,opt,name=hostname,proto3" json:"hostname,omitempty"`
	Port     uint32 `protobuf:"varint,3,opt,name=port,proto3" json:"port,omitempty"`
}

func (x *VideoSlotInfo) Reset() {
	*x = VideoSlotInfo{}
	if protoimpl.UnsafeEnabled {
		mi := &file_videoscaler_service_proto_msgTypes[1]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *VideoSlotInfo) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*VideoSlotInfo) ProtoMessage() {}

func (x *VideoSlotInfo) ProtoReflect() protoreflect.Message {
	mi := &file_videoscaler_service_proto_msgTypes[1]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use VideoSlotInfo.ProtoReflect.Descriptor instead.
func (*VideoSlotInfo) Descriptor() ([]byte, []int) {
	return file_videoscaler_service_proto_rawDescGZIP(), []int{1}
}

func (x *VideoSlotInfo) GetId() string {
	if x != nil {
		return x.Id
	}
	return ""
}

func (x *VideoSlotInfo) GetHostname() string {
	if x != nil {
		return x.Hostname
	}
	return ""
}

func (x *VideoSlotInfo) GetPort() uint32 {
	if x != nil {
		return x.Port
	}
	return 0
}

type SetSlotBranchActiveRequest struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	SlotId string `protobuf:"bytes,1,opt,name=slot_id,json=slotId,proto3" json:"slot_id,omitempty"`
	Name   string `protobuf:"bytes,2,opt,name=name,proto3" json:"name,omitempty"`
	Active bool   `protobuf:"varint,3,opt,name=active,proto3" json:"active,omitempty"`
}

func (x *SetSlotBranchActiveRequest) Reset() {
	*x = SetSlotBranchActiveRequest{}
	if protoimpl.UnsafeEnabled {
		mi := &file_videoscaler_service_proto_msgTypes[2]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *SetSlotBranchActiveRequest) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*SetSlotBranchActiveRequest) ProtoMessage() {}

func (x *SetSlotBranchActiveRequest) ProtoReflect() protoreflect.Message {
	mi := &file_videoscaler_service_proto_msgTypes[2]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use SetSlotBranchActiveRequest.ProtoReflect.Descriptor instead.
func (*SetSlotBranchActiveRequest) Descriptor() ([]byte, []int) {
	return file_videoscaler_service_proto_rawDescGZIP(), []int{2}
}

func (x *SetSlotBranchActiveRequest) GetSlotId() string {
	if x != nil {
		return x.SlotId
	}
	return ""
}

func (x *SetSlotBranchActiveRequest) GetName() string {
	if x != nil {
		return x.Name
	}
	return ""
}

func (x *SetSlotBranchActiveRequest) GetActive() bool {
	if x != nil {
		return x.Active
	}
	return false
}

type ForceKeyFrameRequest struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	SlotId string `protobuf:"bytes,1,opt,name=slot_id,json=slotId,proto3" json:"slot_id,omitempty"`
	Name   string `protobuf:"bytes,2,opt,name=name,proto3" json:"name,omitempty"`
}

func (x *ForceKeyFrameRequest) Reset() {
	*x = ForceKeyFrameRequest{}
	if protoimpl.UnsafeEnabled {
		mi := &file_videoscaler_service_proto_msgTypes[3]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *ForceKeyFrameRequest) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*ForceKeyFrameRequest) ProtoMessage() {}

func (x *ForceKeyFrameRequest) ProtoReflect() protoreflect.Message {
	mi := &file_videoscaler_service_proto_msgTypes[3]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use ForceKeyFrameRequest.ProtoReflect.Descriptor instead.
func (*ForceKeyFrameRequest) Descriptor() ([]byte, []int) {
	return file_videoscaler_service_proto_rawDescGZIP(), []int{3}
}

func (x *ForceKeyFrameRequest) GetSlotId() string {
	if x != nil {
		return x.SlotId
	}
	return ""
}

func (x *ForceKeyFrameRequest) GetName() string {
	if x != nil {
		return x.Name
	}
	return ""
}

type StopVideoSlotRequest struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	SlotId string `protobuf:"bytes,1,opt,name=slot_id,json=slotId,proto3" json:"slot_id,omitempty"`
}

func (x *StopVideoSlotRequest) Reset() {
	*x = StopVideoSlotRequest{}
	if protoimpl.UnsafeEnabled {
		mi := &file_videoscaler_service_proto_msgTypes[4]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *StopVideoSlotRequest) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*StopVideoSlotRequest) ProtoMessage() {}

func (x *StopVideoSlotRequest) ProtoReflect() protoreflect.Message {
	mi := &file_videoscaler_service_proto_msgTypes[4]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use StopVideoSlotRequest.ProtoReflect.Descriptor instead.
func (*StopVideoSlotRequest) Descriptor() ([]byte, []int) {
	return file_videoscaler_service_proto_rawDescGZIP(), []int{4}
}

func (x *StopVideoSlotRequest) GetSlotId() string {
	if x != nil {
		return x.SlotId
	}
	return ""
}

var File_videoscaler_service_proto protoreflect.FileDescriptor

var file_videoscaler_service_proto_rawDesc = []byte{
	0x0a, 0x19, 0x76, 0x69, 0x64, 0x65, 0x6f, 0x73, 0x63, 0x61, 0x6c, 0x65, 0x72, 0x5f, 0x73, 0x65,
	0x72, 0x76, 0x69, 0x63, 0x65, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x1a, 0x1b, 0x67, 0x6f, 0x6f,
	0x67, 0x6c, 0x65, 0x2f, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x62, 0x75, 0x66, 0x2f, 0x65, 0x6d, 0x70,
	0x74, 0x79, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x22, 0x5b, 0x0a, 0x15, 0x53, 0x74, 0x61, 0x72,
	0x74, 0x56, 0x69, 0x64, 0x65, 0x6f, 0x53, 0x6c, 0x6f, 0x74, 0x52, 0x65, 0x71, 0x75, 0x65, 0x73,
	0x74, 0x12, 0x23, 0x0a, 0x0d, 0x73, 0x69, 0x6e, 0x6b, 0x5f, 0x68, 0x6f, 0x73, 0x74, 0x6e, 0x61,
	0x6d, 0x65, 0x18, 0x01, 0x20, 0x01, 0x28, 0x09, 0x52, 0x0c, 0x73, 0x69, 0x6e, 0x6b, 0x48, 0x6f,
	0x73, 0x74, 0x6e, 0x61, 0x6d, 0x65, 0x12, 0x1d, 0x0a, 0x0a, 0x73, 0x69, 0x6e, 0x6b, 0x5f, 0x70,
	0x6f, 0x72, 0x74, 0x73, 0x18, 0x02, 0x20, 0x03, 0x28, 0x0d, 0x52, 0x09, 0x73, 0x69, 0x6e, 0x6b,
	0x50, 0x6f, 0x72, 0x74, 0x73, 0x22, 0x4f, 0x0a, 0x0d, 0x56, 0x69, 0x64, 0x65, 0x6f, 0x53, 0x6c,
	0x6f, 0x74, 0x49, 0x6e, 0x66, 0x6f, 0x12, 0x0e, 0x0a, 0x02, 0x69, 0x64, 0x18, 0x01, 0x20, 0x01,
	0x28, 0x09, 0x52, 0x02, 0x69, 0x64, 0x12, 0x1a, 0x0a, 0x08, 0x68, 0x6f, 0x73, 0x74, 0x6e, 0x61,
	0x6d, 0x65, 0x18, 0x02, 0x20, 0x01, 0x28, 0x09, 0x52, 0x08, 0x68, 0x6f, 0x73, 0x74, 0x6e, 0x61,
	0x6d, 0x65, 0x12, 0x12, 0x0a, 0x04, 0x70, 0x6f, 0x72, 0x74, 0x18, 0x03, 0x20, 0x01, 0x28, 0x0d,
	0x52, 0x04, 0x70, 0x6f, 0x72, 0x74, 0x22, 0x61, 0x0a, 0x1a, 0x53, 0x65, 0x74, 0x53, 0x6c, 0x6f,
	0x74, 0x42, 0x72, 0x61, 0x6e, 0x63, 0x68, 0x41, 0x63, 0x74, 0x69, 0x76, 0x65, 0x52, 0x65, 0x71,
	0x75, 0x65, 0x73, 0x74, 0x12, 0x17, 0x0a, 0x07, 0x73, 0x6c, 0x6f, 0x74, 0x5f, 0x69, 0x64, 0x18,
	0x01, 0x20, 0x01, 0x28, 0x09, 0x52, 0x06, 0x73, 0x6c, 0x6f, 0x74, 0x49, 0x64, 0x12, 0x12, 0x0a,
	0x04, 0x6e, 0x61, 0x6d, 0x65, 0x18, 0x02, 0x20, 0x01, 0x28, 0x09, 0x52, 0x04, 0x6e, 0x61, 0x6d,
	0x65, 0x12, 0x16, 0x0a, 0x06, 0x61, 0x63, 0x74, 0x69, 0x76, 0x65, 0x18, 0x03, 0x20, 0x01, 0x28,
	0x08, 0x52, 0x06, 0x61, 0x63, 0x74, 0x69, 0x76, 0x65, 0x22, 0x43, 0x0a, 0x14, 0x46, 0x6f, 0x72,
	0x63, 0x65, 0x4b, 0x65, 0x79, 0x46, 0x72, 0x61, 0x6d, 0x65, 0x52, 0x65, 0x71, 0x75, 0x65, 0x73,
	0x74, 0x12, 0x17, 0x0a, 0x07, 0x73, 0x6c, 0x6f, 0x74, 0x5f, 0x69, 0x64, 0x18, 0x01, 0x20, 0x01,
	0x28, 0x09, 0x52, 0x06, 0x73, 0x6c, 0x6f, 0x74, 0x49, 0x64, 0x12, 0x12, 0x0a, 0x04, 0x6e, 0x61,
	0x6d, 0x65, 0x18, 0x02, 0x20, 0x01, 0x28, 0x09, 0x52, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x22, 0x2f,
	0x0a, 0x14, 0x53, 0x74, 0x6f, 0x70, 0x56, 0x69, 0x64, 0x65, 0x6f, 0x53, 0x6c, 0x6f, 0x74, 0x52,
	0x65, 0x71, 0x75, 0x65, 0x73, 0x74, 0x12, 0x17, 0x0a, 0x07, 0x73, 0x6c, 0x6f, 0x74, 0x5f, 0x69,
	0x64, 0x18, 0x01, 0x20, 0x01, 0x28, 0x09, 0x52, 0x06, 0x73, 0x6c, 0x6f, 0x74, 0x49, 0x64, 0x32,
	0x93, 0x02, 0x0a, 0x0b, 0x56, 0x69, 0x64, 0x65, 0x6f, 0x53, 0x63, 0x61, 0x6c, 0x65, 0x72, 0x12,
	0x38, 0x0a, 0x0e, 0x53, 0x74, 0x61, 0x72, 0x74, 0x56, 0x69, 0x64, 0x65, 0x6f, 0x53, 0x6c, 0x6f,
	0x74, 0x12, 0x16, 0x2e, 0x53, 0x74, 0x61, 0x72, 0x74, 0x56, 0x69, 0x64, 0x65, 0x6f, 0x53, 0x6c,
	0x6f, 0x74, 0x52, 0x65, 0x71, 0x75, 0x65, 0x73, 0x74, 0x1a, 0x0e, 0x2e, 0x56, 0x69, 0x64, 0x65,
	0x6f, 0x53, 0x6c, 0x6f, 0x74, 0x49, 0x6e, 0x66, 0x6f, 0x12, 0x4a, 0x0a, 0x13, 0x53, 0x65, 0x74,
	0x53, 0x6c, 0x6f, 0x74, 0x42, 0x72, 0x61, 0x6e, 0x63, 0x68, 0x41, 0x63, 0x74, 0x69, 0x76, 0x65,
	0x12, 0x1b, 0x2e, 0x53, 0x65, 0x74, 0x53, 0x6c, 0x6f, 0x74, 0x42, 0x72, 0x61, 0x6e, 0x63, 0x68,
	0x41, 0x63, 0x74, 0x69, 0x76, 0x65, 0x52, 0x65, 0x71, 0x75, 0x65, 0x73, 0x74, 0x1a, 0x16, 0x2e,
	0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x62, 0x75, 0x66, 0x2e,
	0x45, 0x6d, 0x70, 0x74, 0x79, 0x12, 0x3e, 0x0a, 0x0d, 0x46, 0x6f, 0x72, 0x63, 0x65, 0x4b, 0x65,
	0x79, 0x46, 0x72, 0x61, 0x6d, 0x65, 0x12, 0x15, 0x2e, 0x46, 0x6f, 0x72, 0x63, 0x65, 0x4b, 0x65,
	0x79, 0x46, 0x72, 0x61, 0x6d, 0x65, 0x52, 0x65, 0x71, 0x75, 0x65, 0x73, 0x74, 0x1a, 0x16, 0x2e,
	0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x62, 0x75, 0x66, 0x2e,
	0x45, 0x6d, 0x70, 0x74, 0x79, 0x12, 0x3e, 0x0a, 0x0d, 0x53, 0x74, 0x6f, 0x70, 0x56, 0x69, 0x64,
	0x65, 0x6f, 0x53, 0x6c, 0x6f, 0x74, 0x12, 0x15, 0x2e, 0x53, 0x74, 0x6f, 0x70, 0x56, 0x69, 0x64,
	0x65, 0x6f, 0x53, 0x6c, 0x6f, 0x74, 0x52, 0x65, 0x71, 0x75, 0x65, 0x73, 0x74, 0x1a, 0x16, 0x2e,
	0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x62, 0x75, 0x66, 0x2e,
	0x45, 0x6d, 0x70, 0x74, 0x79, 0x42, 0x13, 0x5a, 0x11, 0x62, 0x6f, 0x6e, 0x66, 0x69, 0x72, 0x65,
	0x2f, 0x74, 0x72, 0x61, 0x6e, 0x73, 0x63, 0x6f, 0x64, 0x65, 0x62, 0x06, 0x70, 0x72, 0x6f, 0x74,
	0x6f, 0x33,
}

var (
	file_videoscaler_service_proto_rawDescOnce sync.Once
	file_videoscaler_service_proto_rawDescData = file_videoscaler_service_proto_rawDesc
)

func file_videoscaler_service_proto_rawDescGZIP() []byte {
	file_videoscaler_service_proto_rawDescOnce.Do(func() {
		file_videoscaler_service_proto_rawDescData = protoimpl.X.CompressGZIP(file_videoscaler_service_proto_rawDescData)
	})
	return file_videoscaler_service_proto_rawDescData
}

var file_videoscaler_service_proto_msgTypes = make([]protoimpl.MessageInfo, 5)
var file_videoscaler_service_proto_goTypes = []interface{}{
	(*StartVideoSlotRequest)(nil),      // 0: StartVideoSlotRequest
	(*VideoSlotInfo)(nil),              // 1: VideoSlotInfo
	(*SetSlotBranchActiveRequest)(nil), // 2: SetSlotBranchActiveRequest
	(*ForceKeyFrameRequest)(nil),       // 3: ForceKeyFrameRequest
	(*StopVideoSlotRequest)(nil),       // 4: StopVideoSlotRequest
	(*empty.Empty)(nil),                // 5: google.protobuf.Empty
}
var file_videoscaler_service_proto_depIdxs = []int32{
	0, // 0: VideoScaler.StartVideoSlot:input_type -> StartVideoSlotRequest
	2, // 1: VideoScaler.SetSlotBranchActive:input_type -> SetSlotBranchActiveRequest
	3, // 2: VideoScaler.ForceKeyFrame:input_type -> ForceKeyFrameRequest
	4, // 3: VideoScaler.StopVideoSlot:input_type -> StopVideoSlotRequest
	1, // 4: VideoScaler.StartVideoSlot:output_type -> VideoSlotInfo
	5, // 5: VideoScaler.SetSlotBranchActive:output_type -> google.protobuf.Empty
	5, // 6: VideoScaler.ForceKeyFrame:output_type -> google.protobuf.Empty
	5, // 7: VideoScaler.StopVideoSlot:output_type -> google.protobuf.Empty
	4, // [4:8] is the sub-list for method output_type
	0, // [0:4] is the sub-list for method input_type
	0, // [0:0] is the sub-list for extension type_name
	0, // [0:0] is the sub-list for extension extendee
	0, // [0:0] is the sub-list for field type_name
}

func init() { file_videoscaler_service_proto_init() }
func file_videoscaler_service_proto_init() {
	if File_videoscaler_service_proto != nil {
		return
	}
	if !protoimpl.UnsafeEnabled {
		file_videoscaler_service_proto_msgTypes[0].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*StartVideoSlotRequest); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
		file_videoscaler_service_proto_msgTypes[1].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*VideoSlotInfo); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
		file_videoscaler_service_proto_msgTypes[2].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*SetSlotBranchActiveRequest); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
		file_videoscaler_service_proto_msgTypes[3].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*ForceKeyFrameRequest); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
		file_videoscaler_service_proto_msgTypes[4].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*StopVideoSlotRequest); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
	}
	type x struct{}
	out := protoimpl.TypeBuilder{
		File: protoimpl.DescBuilder{
			GoPackagePath: reflect.TypeOf(x{}).PkgPath(),
			RawDescriptor: file_videoscaler_service_proto_rawDesc,
			NumEnums:      0,
			NumMessages:   5,
			NumExtensions: 0,
			NumServices:   1,
		},
		GoTypes:           file_videoscaler_service_proto_goTypes,
		DependencyIndexes: file_videoscaler_service_proto_depIdxs,
		MessageInfos:      file_videoscaler_service_proto_msgTypes,
	}.Build()
	File_videoscaler_service_proto = out.File
	file_videoscaler_service_proto_rawDesc = nil
	file_videoscaler_service_proto_goTypes = nil
	file_videoscaler_service_proto_depIdxs = nil
}