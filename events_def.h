#ifndef BRKS_COMMON_EVENTS_DEF_H_
#define BRKS_COMMON_EVENTS_DEF_H_

#include "ievent.h"
#include <string>
#include "bike.pb.h"



class MobileCodeReqEv : public iEvent {
public:
	MobileCodeReqEv(const std::string& mobile) :iEvent(EVENTID_GET_MOBILE_CODE_REQ, generateSeqNo())
	{
		msg_.set_mobile(mobile);
	};

	const std::string& get_mobile() { return msg_.mobile(); };
	virtual  std::ostream& dump(std::ostream& out) const;

	virtual i32  ByteSize() { return msg_.ByteSize(); };
	virtual bool SerializeToArray(char* buf, int len) { return msg_.SerializeToArray(buf, len); };
private:
	tutorial::mobile_request msg_;
};


class MobileCodeRspEv : public iEvent {
public:
	MobileCodeRspEv(i32 code, i32 icode) :iEvent(EVENTID_GET_MOBILE_CODE_RSP, generateSeqNo()) { //´íÎó´úºÅ£¬ ÑéÖ¤Âë
		msg_.set_code(code);
		msg_.set_icode(icode);
		msg_.set_data(getReasonByErrorCode(code));
	};

	const i32 get_code() { return msg_.code(); }
	const i32 get_icode() { return msg_.icode(); }
	const std::string& get_data() { return msg_.data(); };
	virtual std::ostream& dump(std::ostream& out)const;

	virtual i32  ByteSize() { return msg_.ByteSize(); };
	virtual bool SerializeToArray(char* buf, int len) { return msg_.SerializeToArray(buf, len); };
private:
	tutorial::mobile_response msg_;
};

class ExitRspEv : public iEvent {
public:
	ExitRspEv() :iEvent(EVENTID_EXIT_RSP, iEvent::generateSeqNo()) {

	}

};


class LoginReqEv : public iEvent {
public:
	LoginReqEv(const std::string& mobile, i32 icode) :iEvent(EVENTID_LOIGIN_REQ, generateSeqNo())
	{
		msg_.set_mobile(mobile);
		msg_.set_icode(icode);
	};

	const std::string& get_mobile() { return msg_.mobile(); };
	const i32& get_icode() { return msg_.icode(); };
	virtual  std::ostream& dump(std::ostream& out) const;

	virtual i32  ByteSize() { return msg_.ByteSize(); };
	virtual bool SerializeToArray(char* buf, int len) { return msg_.SerializeToArray(buf, len); };
private:
	tutorial::login_request msg_;
};


class LoginResEv : public iEvent {
public:
	LoginResEv(i32 code) :iEvent(EVENTID_LOGIN_RSP, generateSeqNo()) { //´íÎó´úºÅ
		msg_.set_code(code);
		
		msg_.set_data(getReasonByErrorCode(code));
	};

	const i32 get_code() { return msg_.code(); }
	const std::string& get_data() { return msg_.data(); };
	virtual std::ostream& dump(std::ostream& out)const;

	virtual i32  ByteSize() { return msg_.ByteSize(); };
	virtual bool SerializeToArray(char* buf, int len) { return msg_.SerializeToArray(buf, len); };
private:
	tutorial::mobile_response msg_;
};

#endif