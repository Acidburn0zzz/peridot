// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'package:apps.modular.lib.app.dart/app.dart';
import 'package:apps.modular.services.application/service_provider.fidl.dart';
import 'package:apps.modular.services.document_store/document.fidl.dart';
import 'package:apps.modular.services.story/link.fidl.dart';
import 'package:apps.modular.services.story/module.fidl.dart';
import 'package:apps.modular.services.story/module_controller.fidl.dart';
import 'package:apps.modular.services.story/story.fidl.dart';
import 'package:apps.mozart.lib.flutter/child_view.dart';
import 'package:apps.mozart.services.views/view_token.fidl.dart';
import 'package:flutter/material.dart';
import 'package:lib.fidl.dart/bindings.dart';

final ApplicationContext _context = new ApplicationContext.fromStartupInfo();

final GlobalKey<_HomeScreenState> _homeKey = new GlobalKey<_HomeScreenState>();

final String _kDocId = 'counter-doc-id';
final String _kCounterValueKey = 'counter-key';

ModuleImpl _module;

ChildViewConnection _conn;

void _log(String msg) {
  print('[Counter Parent] $msg');
}

class LinkWatcherImpl extends LinkWatcher {
  final LinkWatcherBinding _binding = new LinkWatcherBinding();

  /// Gets the [InterfaceHandle] for this [LinkWatcher] implementation.
  ///
  /// The returned handle should only be used once.
  InterfaceHandle<LinkWatcher> getHandle() => _binding.wrap(this);

  /// Correctly close the Link Binding
  void close() => _binding.close();

  /// A callback called whenever the associated [Link] has new changes.
  @override
  void notify(Map<String, Document> docs) {
    _log('LinkWatcherImpl::Notify call');
    docs.keys.forEach((String id) {
      Document doc = docs[id];
      _log('Printing document with id: ${doc.docid}');

      doc.properties.keys.forEach((String key) {
        _log('$key: ${doc.properties[key]}');
      });
    });

    _homeKey.currentState?.updateDoc(docs[_kDocId]);
  }
}

class ModuleImpl extends Module {
  final ModuleBinding _binding = new ModuleBinding();

  final StoryProxy _story = new StoryProxy();
  final LinkProxy _link = new LinkProxy();
  final LinkProxy _linkForChild = new LinkProxy();
  final LinkWatcherImpl _linkWatcher = new LinkWatcherImpl();

  void bind(InterfaceRequest<Module> request) {
    _binding.bind(this, request);
  }

  @override
  void initialize(
      InterfaceHandle<Story> storyHandle,
      InterfaceHandle<Link> linkHandle,
      InterfaceHandle<ServiceProvider> incomingServices,
      InterfaceRequest<ServiceProvider> outgoingServices) {
    _log('ModuleImpl::initialize call');

    // Bind the provided handles to our proxy objects.
    _story.ctrl.bind(storyHandle);
    _link.ctrl.bind(linkHandle);

    // Register the link watcher.
    _link.watchAll(_linkWatcher.getHandle());

    // Duplicate the link handle to pass it down to the sub-module.
    _link.dup(_linkForChild.ctrl.request());

    InterfacePair<ModuleController> moduleControllerPair =
        new InterfacePair<ModuleController>();
    InterfacePair<ViewOwner> viewOwnerPair = new InterfacePair<ViewOwner>();

    // Start the folder list module.
    _story.startModule(
      'file:///system/apps/example_flutter_counter_child',
      _linkForChild.ctrl.unbind(),
      null,
      null,
      moduleControllerPair.passRequest(),
      viewOwnerPair.passRequest(),
    );

    _conn = new ChildViewConnection(viewOwnerPair.passHandle());
    _homeKey.currentState?.updateUI();
  }

  @override
  void stop(void callback()) {
    _log('ModuleImpl::stop call');

    // Do some clean up here.
    _linkWatcher.close();
    _linkForChild.ctrl.close();
    _link.ctrl.close();
    _story.ctrl.close();

    // Invoke the callback to signal that the clean-up process is done.
    callback();
  }

  void _setValue(int newValue) {
    _link.addDocuments(<String, Document>{
      _kDocId: new Document.init(_kDocId, <String, Value>{
        _kCounterValueKey: new Value()..intValue = newValue,
      }),
    });
  }
}

class _HomeScreen extends StatefulWidget {
  _HomeScreen({Key key}) : super(key: key);

  @override
  _HomeScreenState createState() => new _HomeScreenState();
}

class _HomeScreenState extends State<_HomeScreen> {
  Document _exampleDoc;

  int get _currentValue {
    if (_exampleDoc == null ||
        _exampleDoc.properties == null ||
        !_exampleDoc.properties.containsKey(_kCounterValueKey) ||
        _exampleDoc.properties[_kCounterValueKey].tag != ValueTag.intValue) {
      return -1;
    }

    return _exampleDoc.properties[_kCounterValueKey].intValue;
  }

  void updateDoc(Document doc) {
    if (doc != null) {
      setState(() => _exampleDoc = doc);
    }
  }

  @override
  Widget build(BuildContext context) {
    List<Widget> children = <Widget>[
      new Text('I am the parent module!'),
      new Text('Current Value: $_currentValue'),
      new Row(
        mainAxisAlignment: MainAxisAlignment.center,
        children: <Widget>[
          new RaisedButton(
            onPressed: _handleIncrease,
            child: new Text('Increase'),
          ),
          new RaisedButton(
            onPressed: _handleDecrease,
            child: new Text('Decrease'),
          ),
        ],
      ),
    ];

    if (_conn != null) {
      children.add(new Expanded(
        flex: 1,
        child: new ChildView(connection: _conn),
      ));
    }

    return new Material(
      color: Colors.orange[200],
      child: new Container(
        child: new Column(children: children),
      ),
    );
  }

  /// Convenient method for others to trigger UI update.
  void updateUI() {
    setState(() {});
  }

  void _handleIncrease() {
    _module._setValue(_currentValue + 1);
  }

  void _handleDecrease() {
    _module._setValue(_currentValue - 1);
  }
}

/// Main entry point to the example parent module.
void main() {
  _log('Parent module started with context: $_context');

  _context.outgoingServices.addServiceForName(
    (InterfaceRequest<Module> request) {
      _log('Received binding request for Module');
      _module = new ModuleImpl()..bind(request);
    },
    Module.serviceName,
  );

  runApp(new MaterialApp(
    title: 'Counter Parent',
    home: new _HomeScreen(key: _homeKey),
    theme: new ThemeData(primarySwatch: Colors.orange),
    debugShowCheckedModeBanner: false,
  ));
}
