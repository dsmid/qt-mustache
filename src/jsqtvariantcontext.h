#ifndef JSQTVARIANTCONTEXT_H
#define JSQTVARIANTCONTEXT_H

#include <mustache.h>
#include <QStringList>
#include <QScriptEngine>
#include <QScriptValue>

namespace Mustache
{

	class JSQtVariantContext : public QtVariantContext
	{
		Q_OBJECT
	public:
		explicit JSQtVariantContext(const QVariant& root, const QString &fileName, const QString& javaScript = "", PartialResolver* resolver = 0);

		virtual QString stringValue(const QString& key);
		virtual bool canEval(const QString& key) const;
		virtual QString eval(const QString& key, const QString& _template, Mustache::Renderer* renderer);

		bool hasJavaScriptError() const;

		QString getJavaScriptError() const;

		static QScriptValue jsRender(QScriptContext *context, QScriptEngine *engine);

	protected:
		QScriptValue createValue(const QVariant& value);
		virtual QString evalKey(const QString& key);

	private:
		QString javaScript;
		QScriptEngine engine;
		bool javaScriptErrorOccured;
		QString javaScriptError;
	};

}

#endif // JSQTVARIANTCONTEXT_H
