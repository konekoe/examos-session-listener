#include <gtkmm/application.h>
#include <gtkmm/messagedialog.h>
#include <dbus-cxx.h>
#include <boost/process.hpp>


struct LogMessage {
  std::string title;
  std::string message;
};

int ShowLogMessage(void* user_data)
{
  LogMessage* msg = reinterpret_cast<LogMessage*>(user_data);
  Gtk::MessageDialog LogMessage(msg->title, /*bool use_markup=*/false, /*MessageType type=*/Gtk::MESSAGE_INFO, /*ButtonsType buttons=*/Gtk::BUTTONS_OK, /*bool modal=*/false);
  LogMessage.set_secondary_text(msg->message);
  LogMessage.set_keep_above(true);
  LogMessage.run();
  delete msg;
}

void ShowLog(std::string val, std::string val2)
{
  LogMessage* msg = new LogMessage;
  msg->title = val;
  msg->message = val2;
  // mandatory, since Gtk is not thread safe. For more info, check
  // https://developer.gnome.org/gdk3/stable/gdk3-Threads.html#gdk3-Threads.description
  gdk_threads_add_idle(ShowLogMessage, msg);
}

DBus::Dispatcher::pointer dispatcher;
DBus::Connection::pointer connection;
DBus::ObjectProxy::pointer object;
DBus::MethodProxy<void, std::string, std::string, int>::pointer ShellCmdReturn;

void RunCmd(std::string cmd)
{
  boost::process::ipstream is;
  boost::process::child c(cmd, boost::process::std_out > is);

  std::stringstream ss;
  std::string line;

  while(c.running() && std::getline(is, line) && !line.empty())
    ss << line << '\n';

  c.wait();

  try
  {
  DBus::MethodProxy<void, std::string, std::string, int>& ShellCmdReturnProxy = *ShellCmdReturn;
  ShellCmdReturnProxy(cmd, output, c.exit_code());
  }
  catch(std::shared_ptr<DBus::Error> er)
  {
    std::err << "DBus ERROR! Error was: " <<  er->message() << std::endl;
  }
}


int main(int argc, char *argv[])
{
  // Create DBus object and connection to the SYSTEM_BUS
  DBus::init();

  dispatcher = DBus::Dispatcher::create();
  connection = dispatcher->create_connection(DBus::BUS_SYSTEM);

  // Create a dbus object, connect to the endpoint of the daemon
  object = connection->create_object_proxy("examos.utils.examtool.server", "/examos/utils/examtool");

  // Subscribe to log-related signals coming from the daemon
  DBus::signal_proxy<void, std::string, std::string>::pointer LogSignalProxyPtr = object->create_signal<void, std::string, std::string>("Examos.Utils.Examtool.Server", "LogSignals");
  DBus::signal_proxy<void, std::string, std::string>& LogSignalProxy = *LogSignalProxyPtr;

  // Subscribe to command & control signals coming from the daemon
  DBus::signal_proxy<void, std::string>::pointer ShellCmdSignalProxyPtr = object->create_signal<void, std::string>("Examos.Utils.Examtool.Server", "ShellCmdSignals");
  DBus::signal_proxy<void, std::string>& ShellCmdSignalProxy = *ShellCmdSignalProxyPtr;
  ShellCmdReturn = object->create_method<void, std::string, std::string, int>("Examos.Utils.Examtool.Server", "ShellCmdReturn");

  // Connect proxy endpoints to functions
  LogSignalProxy.connect(sigc::ptr_fun(ShowLog));
  ShellCmdSignalProxy.connect(sigc::ptr_fun(RunCmd));

  // Create the Gtkmm application and let it run its busy loop, this should never return!
  auto app = Gtk::Application::create();
  app->hold();

  return app->run();
}