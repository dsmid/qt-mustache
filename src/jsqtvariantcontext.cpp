#include "jsqtvariantcontext.h"

#include <QFile>
#include <QStringList>

namespace Mustache
{

	JSQtVariantContext::JSQtVariantContext(const QVariant &root, const QString &fileName, const QString &javaScript, PartialResolver *resolver)
		: QtVariantContext(root, resolver), javaScript(javaScript), javaScriptErrorOccured(false), javaScriptError("")
	{
		if (javaScript.isEmpty() && !fileName.isEmpty())
		{
			QFile js(fileName);
			if (js.open(QIODevice::ReadOnly))
			{
				this->javaScript = QString::fromUtf8(js.readAll());
			}
			else
			{
				javaScriptError = QString("Unable to open script file %1").arg(fileName);
				javaScriptErrorOccured = true;
				return;
			}
		}
		QScriptSyntaxCheckResult result = QScriptEngine::checkSyntax(this->javaScript);
		switch (result.state())
		{
			case QScriptSyntaxCheckResult::Intermediate:
				javaScriptError = "The script is incomplete";
				javaScriptErrorOccured = true;
				break;
			case QScriptSyntaxCheckResult::Error:
				javaScriptError = QString("JavaScript syntax error on line %1:%2: %3").arg(result.errorLineNumber()).arg(result.errorColumnNumber()).arg(result.errorMessage());
				javaScriptErrorOccured = true;
				break;
			default:
				break;
		}

		if (!javaScriptErrorOccured)
		{
			engine.evaluate(this->javaScript, fileName);
			if (engine.hasUncaughtException())
			{
				javaScriptErrorOccured = true;
				javaScriptError = QString("JavaScript exception on line %1: %2\nBacktrace:\n%3").arg(engine.uncaughtExceptionLineNumber()).arg(engine.uncaughtException().toString()).arg(engine.uncaughtExceptionBacktrace().join("\n"));
			}
		}
	}

	QString JSQtVariantContext::stringValue(const QString &key)
	{
		if (canEval(key))
		{
			return evalKey(key);
		}
		else
		{
			return Mustache::QtVariantContext::stringValue(key);
		}
	}

	bool JSQtVariantContext::canEval(const QString &key) const
	{
		if (javaScriptErrorOccured)
		{
			return false;
		}
		QScriptValue global = engine.globalObject();
		QScriptValue fun = global.property(key);
		if (fun.isValid() && fun.isFunction())
		{
			return true;
		}
		return false;
	}


	QString JSQtVariantContext::evalKey(const QString &key)
	{
		QScriptValue global = engine.globalObject();
		QScriptValue fun = global.property(key);
		if (fun.isValid() && fun.isFunction())
		{
			QScriptValue thi = createValue(value("."));
			QScriptValueList args;
			QScriptValue res = fun.call(thi, args);
			if (res.isValid())
			{
				if (!res.isError())
				{
					return res.toString();
				}
			}
		}
		return "";
	}

	QString JSQtVariantContext::eval(const QString &key, const QString &_template, Renderer *renderer)
	{
		QScriptValue qs_renderer = engine.newQObject(renderer);
		QScriptValue qs_context = engine.newQObject(this);

		// For render() AKA jsRender() function
		engine.globalObject().setProperty("renderer", qs_renderer);
		engine.globalObject().setProperty("context", qs_context);

		QScriptValue fun = engine.globalObject().property(key);
		if (fun.isValid() && fun.isFunction())
		{
			QScriptValue thi = createValue(value("."));
			QScriptValueList args;
			args << _template;
			args << engine.newFunction(jsRender);
			QScriptValue res = fun.call(thi, args);
			if (engine.hasUncaughtException())
			{
				javaScriptErrorOccured = true;
				javaScriptError = QString("JavaScript exception on line %1: %2\nBacktrace:\n%3").arg(engine.uncaughtExceptionLineNumber()).arg(engine.uncaughtException().toString()).arg(engine.uncaughtExceptionBacktrace().join("\n"));
			}
			if (res.isValid())
			{
				if (!res.isError())
				{
					return res.toString();
				}
			}
		}
		return "";
	}

	bool JSQtVariantContext::hasJavaScriptError() const
	{
		return javaScriptErrorOccured;
	}

	QString JSQtVariantContext::getJavaScriptError() const
	{
		return javaScriptError;
	}

	QScriptValue JSQtVariantContext::jsRender(QScriptContext *context, QScriptEngine *engine)
	{
		if (context->argumentCount() >= 1)
		{
			QScriptValue qs_renderer = engine->globalObject().property("renderer");
			Renderer * renderer = qobject_cast<Renderer *>(qs_renderer.toQObject());

			QScriptValue qs_context = engine->globalObject().property("context");
			JSQtVariantContext * qt_context = qobject_cast<JSQtVariantContext *>(qs_context.toQObject());

			if (renderer != NULL && qt_context != NULL)
			{
				return QScriptValue(renderer->render(context->argument(0).toString(), qt_context));
			}
		}
		return "";
	}


	QScriptValue JSQtVariantContext::createValue(const QVariant& value)
	{
		if(value.type() == QVariant::Map)
		{
			QScriptValue obj = engine.newObject();

			QVariantMap map = value.toMap();
			QVariantMap::const_iterator it = map.begin();
			QVariantMap::const_iterator end = map.end();
			while(it != end)
			{
				obj.setProperty( it.key(), createValue(it.value()) );
				++it;
			}

			return obj;
		}

		if(value.type() == QVariant::Hash)
		{
			QScriptValue obj = engine.newObject();

			QVariantHash hash = value.toHash();
			QVariantHash::const_iterator it = hash.begin();
			QVariantHash::const_iterator end = hash.end();
			while(it != end)
			{
				obj.setProperty( it.key(), createValue(it.value()) );
				++it;
			}

			return obj;
		}

		if(value.type() == QVariant::List)
		{
			QVariantList list = value.toList();
			QScriptValue array = engine.newArray(static_cast<uint>(list.length()));
			for(int i=0; i<list.count(); i++)
				array.setProperty(static_cast<quint32>(i), createValue(list.at(i)));

			return array;
		}

		switch(value.type())
		{
		case QVariant::String:
			return QScriptValue(value.toString());
		case QVariant::Int:
			return QScriptValue(value.toInt());
		case QVariant::UInt:
			return QScriptValue(value.toUInt());
		case QVariant::Bool:
			return QScriptValue(value.toBool());
		case QVariant::ByteArray:
			return QScriptValue(QLatin1String(value.toByteArray()));
		case QVariant::Double:
			return QScriptValue(static_cast<qsreal>(value.toDouble()));
		default:
			break;
		}

		if(value.isNull())
			return QScriptValue(QScriptValue::NullValue);

		return engine.newVariant(value);
	}

}
