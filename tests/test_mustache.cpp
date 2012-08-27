/*
  Copyright 2012, Robert Knight

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

    Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
*/

#include "test_mustache.h"

void TestMustache::testValues()
{
	QVariantMap map;
	map["name"] = "John Smith";
	map["age"] = 42;
	map["sex"] = "Male";
	map["company"] = "Smith & Co";
	map["signature"] = "John Smith of <b>Smith & Co</b>";

	QString _template = "Name: {{name}}, Age: {{age}}, Sex: {{sex}}\n"
	                    "Company: {{company}}\n"
						"  {{{signature}}}"
	                    "{{missing-key}}";
	QString expectedOutput = "Name: John Smith, Age: 42, Sex: Male\n"
	                         "Company: Smith &amp; Co\n"
	                         "  John Smith of <b>Smith & Co</b>";

	Mustache::Renderer renderer;
	Mustache::QtVariantContext context(map);
	QString output = renderer.render(_template, &context);

	QCOMPARE(output, expectedOutput);
}

QVariantMap contactInfo(const QString& name, const QString& email)
{
	QVariantMap map;
	map["name"] = name;
	map["email"] = email;
	return map;
}

void TestMustache::testSections()
{
	QVariantMap map = contactInfo("John Smith", "john.smith@gmail.com");
	QVariantList contacts;
	contacts << contactInfo("James Dee", "james@dee.org");
	contacts << contactInfo("Jim Jones", "jim-jones@yahoo.com");
	map["contacts"] = contacts;

	QString _template = "Name: {{name}}, Email: {{email}}\n"
	                    "{{#contacts}}  {{name}} - {{email}}\n{{/contacts}}"
						"{{^contacts}}  No contacts{{/contacts}}";

	QString expectedOutput = "Name: John Smith, Email: john.smith@gmail.com\n"
	                         "  James Dee - james@dee.org\n"
	                         "  Jim Jones - jim-jones@yahoo.com\n";

	Mustache::Renderer renderer;
	Mustache::QtVariantContext context(map);
	QString output = renderer.render(_template, &context);

	QCOMPARE(output, expectedOutput);

	// test inverted sections
	map.remove("contacts");
	context = Mustache::QtVariantContext(map);
	output = renderer.render(_template, &context);

	expectedOutput = "Name: John Smith, Email: john.smith@gmail.com\n"
	                 "  No contacts";
	QCOMPARE(output, expectedOutput);

	// test with an empty list instead of an empty key
	map["contacts"] = QVariantMap();
	context = Mustache::QtVariantContext(map);
	output = renderer.render(_template, &context);
	QCOMPARE(output, expectedOutput);
}

void TestMustache::testPartials()
{
	QHash<QString, QString> partials;
	partials["file-info"] = "{{name}} {{size}} {{type}}\n";

	QString _template = "{{#files}}{{>file-info}}{{/files}}";

	QVariantMap map;
	QVariantList fileList;
	
	QVariantMap file1;
	file1["name"] = "mustache.pdf";
	file1["size"] = "200KB";
	file1["type"] = "PDF Document";

	QVariantMap file2;
	file2["name"] = "cv.doc";
	file2["size"] = "300KB";
	file2["type"] = "Microsoft Word Document";

	fileList << file1 << file2;
	map["files"] = fileList;

	Mustache::Renderer renderer;
	Mustache::PartialMap partialMap(partials);
	Mustache::QtVariantContext context(map, &partialMap);
	QString output = renderer.render(_template, &context);

	QCOMPARE(output,
	 QString("mustache.pdf 200KB PDF Document\n"
	         "cv.doc 300KB Microsoft Word Document\n"));
}

void TestMustache::testSetDelimiters()
{
	QVariantMap map;
	map["name"] = "John Smith";
	map["phone"] = "01234 567890";

	QString _template =
	  "{{=<% %>=}}"
	  "<%name%>{{ }}<%phone%>"
	  "<%={{ }}=%>"
	  " {{name}}<% %>{{phone}}";

	QString expectedOutput = "John Smith{{ }}01234 567890 John Smith<% %>01234 567890";
	
	Mustache::Renderer renderer;
	Mustache::QtVariantContext context(map);
	QString output = renderer.render(_template, &context);

	QCOMPARE(output, expectedOutput);
}

void TestMustache::testErrors()
{
	QVariantMap map;
	map["name"] = "Jim Jones";

	QHash<QString, QString> partials;
	partials["buggy-partial"] = "--{{/one}}--";

	QString _template = "{{name}}";

	Mustache::Renderer renderer;
	Mustache::PartialMap partialMap(partials);
	Mustache::QtVariantContext context(map, &partialMap);
	QString output = renderer.render(_template, &context);

	QCOMPARE(output, QString("Jim Jones"));
	QCOMPARE(renderer.error(), QString());
	QCOMPARE(renderer.errorPos(), -1);

	_template = "{{#one}} {{/two}}";
	output = renderer.render(_template, &context);
	QCOMPARE(renderer.error(), QString("Tag start/end key mismatch"));
	QCOMPARE(renderer.errorPos(), 9);
	QCOMPARE(renderer.errorPartial(), QString());

	_template = "Hello {{>buggy-partial}}";
	output = renderer.render(_template, &context);
	QCOMPARE(renderer.error(), QString("Unexpected end tag"));
	QCOMPARE(renderer.errorPos(), 2);
	QCOMPARE(renderer.errorPartial(), QString("buggy-partial"));
}

QTEST_APPLESS_MAIN(TestMustache)
