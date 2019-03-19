#include <widgets/qpalette_inner.h>
#include <widgets/palette_manager.h>

#include <bindings/pypalette.h>

namespace detail {
    template <typename F>
    struct FEvent : public QEvent {
        using Fun = typename std::decay<F>::type;
        Fun fun;
        FEvent(Fun&& fun) : QEvent(QEvent::None), fun(std::move(fun)) {}
        FEvent(const Fun& fun) : QEvent(QEvent::None), fun(fun) {}
        ~FEvent() { fun(); }
    };
}

template <typename F>
static void postToObject(F&& fun, QObject* obj = qApp) {
    if (qobject_cast<QThread*>(obj))
        qWarning() << "posting a call to a thread object - consider using postToThread";
    QCoreApplication::postEvent(obj, new detail::FEvent<F>(std::forward<F>(fun)));
}

template <typename F>
static void postToThread(F&& fun, QThread* thread = qApp->thread()) {
    QObject* obj = QAbstractEventDispatcher::instance(thread);
    Q_ASSERT(obj);
    QCoreApplication::postEvent(obj, new detail::FEvent<F>(std::forward<F>(fun)));
}

PyPalette::PyPalette(std::string& name, py::list entries) {
    QVector<Action> result;

    name_ = QString::fromStdString(name);
    result.reserve(static_cast<int>(entries.size()));

    for (int i = 0; i < entries.size(); i++) {
        py::handle item = entries[i];
        result.push_back(Action(
            QString::fromStdString(item.attr("id").cast<std::string>()),
            QString::fromStdString(item.attr("description").cast<std::string>()),
            QString::fromStdString(item.attr("shortcut").cast<std::string>())
        ));
    }

    py::object scope = py::module::import("__palette__").attr("__dict__");

    scope["__entries__"] = entries;

    py::exec(
        "__cur_palette__ = {l.id: l for l in __entries__}",
        scope);

    actions_.swap(result);
}

PYBIND11_MODULE(__palette__, m) {
    m.doc() = R"()";

    py::class_<PyPalette>(m, "Palette")
        .def(py::init<std::string, py::list>());

    m.def("show_palette", [](PyPalette & palette) {
        show_palette(palette.name(), palette.actions(), [](const Action & action) {
            py::gil_scoped_acquire gil;

            try {
                py::object trigger_action_py = py::module::import("__palette__").attr("execute_action");
                auto res = trigger_action_py(action.id().toStdString());

                //if (py::isinstance<PyPalette>(res)) {
                //    EnterResult enter_result = py::cast<EnterResult>(res);
                //    return enter_result;
                //}
                //else {
                //    return EnterResult(res == Py_True);
                //}

                return true;
            }
            catch (const std::runtime_error & error) {
                // This should not throw error
                auto write = py::module::import("sys").attr("stdout").attr("write");
                write(error.what());
                write("\n");
                PyErr_Clear();
            }

            return true;
            }); });

    m.attr("threading") = py::module::import("threading");

    py::exec(R"(
class Action:
	def __init__(self, id, description, handler):
		self.id = id
		self.description = description
		self.shortcut = ""
		self.handler = handler

def execute_action(id):
    threading.Thread(target=__cur_palette__[id].handler, args=(__cur_palette__[id], )).start()

)", m.attr("__dict__"));

    m.attr("__version__") = "dev";
}

void init_python_module() {
    init__palette__();
}

