#include "events_def.h"
#include <iostream>
#include <sstream>



std::ostream& MobileCodeReqEv::dump(std::ostream& out) const
{
	out << "MoblieCodeReq sn=" << get_sn() << ",";
	out << "moblie=" << msg_.mobile() << std::endl << std::endl;
	return out;
}

std::ostream& MobileCodeRspEv::dump(std::ostream& out) const
{
	out << "MoblieCodeRsp sn=" << get_sn() << ",";
	out << "code=" << msg_.code() << std::endl;
	out << "icode=" << msg_.icode() << std::endl;    //手机验证码
	out << "describe=" << msg_.data() << std::endl << std::endl;

	return out;
}

std::ostream& LoginReqEv::dump(std::ostream& out) const
{
	out << "LoginReq sn=" << get_sn() << ",";
	out << "moblie=" << msg_.mobile() << std::endl << std::endl;
	out << "icode=" << msg_.icode() << std::endl << std::endl;
	return out;
}

std::ostream& LoginResEv::dump(std::ostream& out) const
{
	out << "LoginRes sn=" << get_sn() << ",";
	out << "code=" << msg_.code() << std::endl;
	out << "describe=" << msg_.data() << std::endl << std::endl;

	return out;
}
