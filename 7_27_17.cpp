#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <experimental/filesystem>
#include <vector>
#include <tuple>

#include <phidget22.h>

#ifndef _WIN32
#include <unistd.h>
#define SLEEP(tm) sleep(tm)
#else
#include <Windows.h>
#define SLEEP(tm) Sleep(tm * 1000)
#endif

static void CCONV onAttachHandler(PhidgetHandle phid, void *) {
	PhidgetReturnCode res;
	int hubPort;
	int channel;
	int serial;

	res = Phidget_getDeviceSerialNumber(phid, &serial);
	if (res != EPHIDGET_OK) {
		std::cerr << "failed to get device serial number" << std::endl;
		return;
	}

	res = Phidget_getChannel(phid, &channel);
	if (res != EPHIDGET_OK) {
		std::cerr << "failed to get channel number" << std::endl;
		return;
	}

	res = Phidget_getHubPort(phid, &hubPort);
	if (res != EPHIDGET_OK) {
		std::cerr << "failed to get hub port" << std::endl;
		hubPort = -1;
	}

	if (hubPort == -1)
		std::cout << "channel " << channel << " on device " << serial << " attached" << std::endl;
	else
		std::cout << "channel " << channel << " on device " << serial << " hub port " << hubPort << " attached" << std::endl;
}

static void CCONV onDetachHandler(PhidgetHandle phid, void *) {
	PhidgetReturnCode res;
	int hubPort;
	int channel;
	int serial;

	res = Phidget_getDeviceSerialNumber(phid, &serial);
	if (res != EPHIDGET_OK) {
		std::cerr << "failed to get device serial number" << std::endl;
		return;
	}

	res = Phidget_getChannel(phid, &channel);
	if (res != EPHIDGET_OK) {
		std::cerr << "failed to get channel number" << std::endl;
		return;
	}

	res = Phidget_getHubPort(phid, &hubPort);
	if (res != EPHIDGET_OK)
		hubPort = -1;

	if (hubPort != -1)
		std::cout << "channel " << channel << " on device " << serial << " detached" << std::endl;
	else
		std::cout << "channel " << channel << " on device " << serial << " hub port " << hubPort << " detached" << std::endl;
}

static void CCONV errorHandler(PhidgetHandle, void *, Phidget_ErrorEventCode errorCode, const char *errorString) {
	std::cerr << "Error: " << errorString << " (" << errorCode << ')' << std::endl;
}

static void CCONV onSpatialDataHandler(PhidgetSpatialHandle, void *, const double* acceleration, const double* angularRate, const double* magneticField, double timestamp) {
	std::cout
		<< std::setprecision(15)
		<< "Acceleration Changed: " << acceleration[0] << ", " << acceleration[1] << ", " << acceleration[2] << std::endl
		<< "Angular Rate Changed: " << angularRate[0] << ", " << angularRate[1] << ", " << angularRate[2] << std::endl
		<< "Magnetic Field Changed: " << magneticField[0] << ", " << magneticField[1] << ", " << magneticField[2] << std::endl
		<< "Timestamp: " << timestamp << std::endl
		<< std::endl;
}

/*
* Creates and initializes the channel.
*/
static PhidgetReturnCode CCONV initChannel(PhidgetHandle ch) {
	PhidgetReturnCode res;

	res = Phidget_setOnAttachHandler(ch, onAttachHandler, NULL);
	if (res != PhidgetReturnCode::EPHIDGET_OK) {
		std::cerr << "failed to assign on attach handler" << std::endl;
		return (res);
	}

	res = Phidget_setOnDetachHandler(ch, onDetachHandler, NULL);
	if (res != PhidgetReturnCode::EPHIDGET_OK) {
		std::cerr << "failed to assign on detach handler" << std::endl;
		return (res);
	}

	res = Phidget_setOnErrorHandler(ch, errorHandler, NULL);
	if (res != PhidgetReturnCode::EPHIDGET_OK) {
		std::cerr << "failed to assign on error handler" << std::endl;
		return (res);
	}

	/*
	* Please review the Phidget22 channel matching documentation for details on the device
	* and class architecture of Phidget22, and how channels are matched to device features.
	*/

	/*
	* Specifies the serial number of the device to attach to.
	* For VINT devices, this is the hub serial number.
	*
	* The default is any device.
	*/
	// Phidget_setDeviceSerialNumber(ch, <YOUR DEVICE SERIAL NUMBER>); 

	/*
	* For VINT devices, this specifies the port the VINT device must be plugged into.
	*
	* The default is any port.
	*/
	// Phidget_setHubPort(ch, 0);

	/*
	* Specifies which channel to attach to.  It is important that the channel of
	* the device is the same class as the channel that is being opened.
	*
	* The default is any channel.
	*/
	// Phidget_setChannel(ch, 0);

	/*
	* In order to attach to a network Phidget, the program must connect to a Phidget22 Network Server.
	* In a normal environment this can be done automatically by enabling server discovery, which
	* will cause the client to discovery and connect to available servers.
	*
	* To force the channel to only match a network Phidget, set remote to 1.
	*/
	// PhidgetNet_enableServerDiscovery(PHIDGETSERVER_DEVICE);
	// Phidget_setIsRemote(ch, 1);

	return PhidgetReturnCode::EPHIDGET_OK;
}

namespace phidget {
	using ReturnCode = PhidgetReturnCode;
	using Handle = PhidgetHandle;
	using SpatialHandle = PhidgetSpatialHandle;

	std::string get_error_description(ReturnCode code) {
		const char *error_c_string;
		auto return_code = Phidget_getErrorDescription(code, &error_c_string);

		if (return_code != ReturnCode::EPHIDGET_OK) {
			std::ostringstream oss;
			oss << "phidget::get_error_description: error code " << return_code;
			throw std::runtime_error(oss.str());
		}

		return error_c_string;
	}

	void error_handler(ReturnCode code) {
		if (code == ReturnCode::EPHIDGET_OK) {
			return;
		}

		std::ostringstream oss;
		oss << "phidget::error_handler: ";

		try {
			auto description = get_error_description(code);

			oss << description << ' ';
		} catch (const std::exception &e) {
			oss << "unable to get error description (" << e.what() << ") ";
		}

		oss << '(' << code << ')';

		throw std::runtime_error(oss.str());
	}

	template <typename T>
	struct HandleWrapper {
		T base;

		Handle as_handle() const {
			return reinterpret_cast<Handle>(base);
		}
	};

	class Spatial {
		HandleWrapper<SpatialHandle> handle;

	public:
		Spatial() {
			try {
				error_handler(PhidgetSpatial_create(&handle.base));
			} catch (const std::exception &e) {
				std::ostringstream oss;
				oss << "phidget::Spatial::Spatial: failed to create Spatial handle: " << e.what();
				throw std::runtime_error(oss.str());
			}

			try {
				error_handler(initChannel(handle.as_handle()));
			} catch (const std::exception &e) {
				std::ostringstream oss;
				oss << "phidget::Spatial::Spatial: failed to initialize channel: " << e.what();
				throw std::runtime_error(oss.str());
			}

			try {
				error_handler(PhidgetSpatial_setOnSpatialDataHandler(handle.base, onSpatialDataHandler, NULL));
			} catch (const std::exception &e) {
				std::ostringstream oss;
				oss << "phidget::Spatial::Spatial: failed to set spatial data change handler: " << e.what();
				throw std::runtime_error(oss.str());
			}
		}

		~Spatial() {
			Phidget_close(handle.as_handle());
			PhidgetSpatial_delete(&handle.base);
		}

		void open_wait_for_attachment(std::uint32_t timeout_ms) {
			try {
				error_handler(Phidget_openWaitForAttachment(handle.as_handle(), timeout_ms));
			} catch (const std::exception &e) {
				std::ostringstream oss;
				oss << "phidget::Spatial::open_wait_for_attachment: failed to open channel: " << e.what();
				throw std::runtime_error(oss.str());
			}
		}
	};
}

template <typename T>
struct Range {
	T first, last;

	Range(T first, T last) : first(first), last(last) { }

	template <typename U>
	Range(U &&x) : first(std::begin(std::forward<U>(x))), last(std::end(std::forward<U>(x))) { }
};

template <typename T>
auto make_range(T &&x) {
	using Iterator = decltype(std::begin(std::forward<T>(x)));
	return Range<Iterator>(x);
}

template <typename Iterator>
auto make_range(Iterator first, Iterator last) {
	return Range<Iterator>(first, last);
}

template <typename T, typename U>
std::ostream& operator<<(std::ostream &os, const std::pair<T, U> &x) {
	return os << '[' << x.first << ", " << x.second << ']';
}

template <typename InputIterator>
std::ostream& operator<<(std::ostream &os, const Range<InputIterator> &iterator_pair) {
	auto first = iterator_pair.first;
	auto last = iterator_pair.last;

	os << '[' << *first++;

	for (; first != last; ++first) {
		os << ", " << *first;
	}

	return os << ']';
}

namespace detail {
	template <typename Tuple, std::size_t N>
	struct print_tuple_impl {
		static std::ostream& apply(std::ostream &os, const Tuple &x) {
			print_tuple_impl<Tuple, N - 1>::apply(os, x);
			return os << ", " << std::get<N - 1>(x);
		}
	};

	template <typename Tuple>
	struct print_tuple_impl<Tuple, 1> {
		static std::ostream& apply(std::ostream &os, const Tuple &x) {
			return os << std::get<0>(x);
		}
	};
}

template <typename... Types>
std::ostream& operator<<(std::ostream &os, const std::tuple<Types...> &x) {
	using Tuple = decltype(x);

	os << '[';
	return detail::print_tuple_impl<Tuple, sizeof...(Types)>::apply(os, x) << ']';
}

class DoubleStream {
	std::ostream &os1, &os2;
public:
	DoubleStream(std::ostream &os1, std::ostream &os2) : os1(os1), os2(os2) { }

	template <typename T>
	friend DoubleStream& operator<<(DoubleStream &ds, T &&x) {
		ds.os1 << std::forward<T>(x);
		ds.os2 << std::forward<T>(x);
		return ds;
	}

	friend DoubleStream& operator<<(DoubleStream &ds, std::streambuf *sb) {
		ds.os1 << sb;
		ds.os2 << sb;
		return ds;
	}

	friend DoubleStream& operator<<(DoubleStream &ds, std::ostream &(*pf)(std::ostream&)) {
		ds.os1 << pf;
		ds.os2 << pf;
		return ds;
	}

	friend DoubleStream& operator<<(DoubleStream &ds, std::ios &(*pf)(std::ios&)) {
		ds.os1 << pf;
		ds.os2 << pf;
		return ds;
	}

	friend DoubleStream& operator<<(DoubleStream &ds, std::ios_base &(*pf)(std::ios_base&)) {
		ds.os1 << pf;
		ds.os2 << pf;
		return ds;
	}
};


int main() {
	std::ofstream ofs("file.txt");
	DoubleStream ds(std::cout, ofs);

	std::vector<int> vec = {0, 1, 2, 3, 4, 5};
	auto pair = std::make_pair(10, 5);
	auto tuple = std::make_tuple("hey there bud", 5.0, 'c');

	ds << "vec = " << make_range(vec) << std::endl;
	ds << "pair = " << pair << std::endl;
	ds << "tuple = " << tuple << std::endl;
	ds << "Hey, what's going on? Tell me all about your life." << std::endl;

	auto this_path = std::experimental::filesystem::current_path();

	ds << "current path: " << this_path << std::endl;

	system("pause");

	//std::ios_base::sync_with_stdio(false);

	//PhidgetLog_enable(PHIDGET_LOG_INFO, NULL);

	//phidget::Spatial spatial;
	//spatial.open_wait_for_attachment(5000);

	//std::cout << "Gathering data for 10 seconds..." << std::endl;
	//SLEEP(10);
}