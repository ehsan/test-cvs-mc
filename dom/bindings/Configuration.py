# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

autogenerated_comment = "/* THIS FILE IS AUTOGENERATED - DO NOT EDIT */\n"

class Configuration:
    """
    Represents global configuration state based on IDL parse data and
    the configuration file.
    """
    def __init__(self, filename, parseData):

        # Read the configuration file.
        glbl = {}
        execfile(filename, glbl)
        config = glbl['DOMInterfaces']

        # Build descriptors for all the interfaces we have in the parse data.
        # This allows callers to specify a subset of interfaces by filtering
        # |parseData|.
        self.descriptors = []
        self.interfaces = {}
        self.maxProtoChainLength = 0;
        for thing in parseData:
            if not thing.isInterface(): continue
            iface = thing
            if iface.identifier.name not in config: continue
            self.interfaces[iface.identifier.name] = iface
            entry = config[iface.identifier.name]
            if not isinstance(entry, list):
                assert isinstance(entry, dict)
                entry = [entry]
            self.descriptors.extend([Descriptor(self, iface, x) for x in entry])

        # Mark the descriptors for which only a single nativeType implements
        # an interface.
        for descriptor in self.descriptors:
            intefaceName = descriptor.interface.identifier.name
            otherDescriptors = [d for d in self.descriptors
                                if d.interface.identifier.name == intefaceName]
            descriptor.uniqueImplementation = len(otherDescriptors) == 1

        self.enums = [e for e in parseData if e.isEnum()]
        self.dictionaries = [d for d in parseData if d.isDictionary()]

        # Keep the descriptor list sorted for determinism.
        self.descriptors.sort(lambda x,y: cmp(x.name, y.name))

    def getInterface(self, ifname):
        return self.interfaces[ifname]
    def getDescriptors(self, **filters):
        """Gets the descriptors that match the given filters."""
        curr = self.descriptors
        for key, val in filters.iteritems():
            if key == 'webIDLFile':
                getter = lambda x: x.interface.filename()
            elif key == 'hasInterfaceObject':
                getter = lambda x: (not x.interface.isExternal() and
                                    x.interface.hasInterfaceObject())
            elif key == 'hasInterfacePrototypeObject':
                getter = lambda x: (not x.interface.isExternal() and
                                    x.interface.hasInterfacePrototypeObject())
            elif key == 'hasInterfaceOrInterfacePrototypeObject':
                getter = lambda x: x.hasInterfaceOrInterfacePrototypeObject()
            elif key == 'isCallback':
                getter = lambda x: x.interface.isCallback()
            elif key == 'isExternal':
                getter = lambda x: x.interface.isExternal()
            else:
                getter = lambda x: getattr(x, key)
            curr = filter(lambda x: getter(x) == val, curr)
        return curr
    def getEnums(self, webIDLFile):
        return filter(lambda e: e.filename() == webIDLFile, self.enums)
    def getDictionaries(self, webIDLFile):
        return filter(lambda d: d.filename() == webIDLFile, self.dictionaries)
    def getDescriptor(self, interfaceName, workers):
        """
        Gets the appropriate descriptor for the given interface name
        and the given workers boolean.
        """
        iface = self.getInterface(interfaceName)
        descriptors = self.getDescriptors(interface=iface)

        # The only filter we currently have is workers vs non-workers.
        matches = filter(lambda x: x.workers is workers, descriptors)

        # After filtering, we should have exactly one result.
        if len(matches) is not 1:
            raise NoSuchDescriptorError("For " + interfaceName + " found " +
                                        str(len(matches)) + " matches");
        return matches[0]
    def getDescriptorProvider(self, workers):
        """
        Gets a descriptor provider that can provide descriptors as needed,
        for the given workers boolean
        """
        return DescriptorProvider(self, workers)

class NoSuchDescriptorError(TypeError):
    def __init__(self, str):
        TypeError.__init__(self, str)

class DescriptorProvider:
    """
    A way of getting descriptors for interface names
    """
    def __init__(self, config, workers):
        self.config = config
        self.workers = workers

    def getDescriptor(self, interfaceName):
        """
        Gets the appropriate descriptor for the given interface name given the
        context of the current descriptor. This selects the appropriate
        implementation for cases like workers.
        """
        return self.config.getDescriptor(interfaceName, self.workers)

class Descriptor(DescriptorProvider):
    """
    Represents a single descriptor for an interface. See Bindings.conf.
    """
    def __init__(self, config, interface, desc):
        DescriptorProvider.__init__(self, config, desc.get('workers', False))
        self.interface = interface

        # Read the desc, and fill in the relevant defaults.
        ifaceName = self.interface.identifier.name
        if self.interface.isExternal() or self.interface.isCallback():
            if self.workers:
                nativeTypeDefault = "JSObject"
            else:
                nativeTypeDefault = "nsIDOM" + ifaceName
        else:
            if self.workers:
                nativeTypeDefault = "mozilla::dom::workers::" + ifaceName
            else:
                nativeTypeDefault = "mozilla::dom::" + ifaceName

        self.nativeType = desc.get('nativeType', nativeTypeDefault)
        self.hasInstanceInterface = desc.get('hasInstanceInterface', None)

        # Do something sane for JSObject
        if self.nativeType == "JSObject":
            headerDefault = "jsapi.h"
        else:
            headerDefault = self.nativeType
            headerDefault = headerDefault.replace("::", "/") + ".h"
        self.headerFile = desc.get('headerFile', headerDefault)

        if self.interface.isCallback() or self.interface.isExternal():
            if 'castable' in desc:
                raise TypeError("%s is external or callback but has a castable "
                                "setting" % self.interface.identifier.name)
            self.castable = False
        else:
            self.castable = desc.get('castable', True)

        self.notflattened = desc.get('notflattened', False)
        self.register = desc.get('register', True)

        self.hasXPConnectImpls = desc.get('hasXPConnectImpls', False)

        # If we're concrete, we need to crawl our ancestor interfaces and mark
        # them as having a concrete descendant.
        self.concrete = desc.get('concrete', not self.interface.isExternal())
        if self.concrete:
            self.proxy = False
            operations = {
                'IndexedGetter': None,
                'IndexedSetter': None,
                'IndexedCreator': None,
                'IndexedDeleter': None,
                'NamedGetter': None,
                'NamedSetter': None,
                'NamedCreator': None,
                'NamedDeleter': None,
                'Stringifier': None
            }
            iface = self.interface
            while iface:
                for m in iface.members:
                    if not m.isMethod():
                        continue

                    def addOperation(operation, m):
                        if not operations[operation]:
                            operations[operation] = m
                    def addIndexedOrNamedOperation(operation, m):
                        self.proxy = True
                        if m.isIndexed():
                            operation = 'Indexed' + operation
                        else:
                            assert m.isNamed()
                            operation = 'Named' + operation
                        addOperation(operation, m)
                        
                    if m.isStringifier():
                        addOperation('Stringifier', m)
                    else:
                        if m.isGetter():
                            addIndexedOrNamedOperation('Getter', m)
                        if m.isSetter():
                            addIndexedOrNamedOperation('Setter', m)
                        if m.isCreator():
                            addIndexedOrNamedOperation('Creator', m)
                        if m.isDeleter():
                            addIndexedOrNamedOperation('Deleter', m)
                            raise TypeError("deleter specified on %s but we "
                                            "don't support deleters yet" %
                                            self.interface.identifier.name)

                iface.setUserData('hasConcreteDescendant', True)
                iface = iface.parent

            if self.proxy:
                self.operations = operations
                iface = self.interface
                while iface:
                    iface.setUserData('hasProxyDescendant', True)
                    iface = iface.parent

        if self.interface.isExternal() and 'prefable' in desc:
            raise TypeError("%s is external but has a prefable setting" %
                            self.interface.identifier.name)
        self.prefable = desc.get('prefable', False)

        self.nativeIsISupports = not self.workers
        self.customTrace = desc.get('customTrace', self.workers)
        self.customFinalize = desc.get('customFinalize', self.workers)
        self.wrapperCache = self.workers or desc.get('wrapperCache', True)

        if not self.wrapperCache and self.prefable:
            raise TypeError("Descriptor for %s is prefable but not wrappercached" %
                            self.interface.identifier.name)

        def make_name(name):
            return name + "_workers" if self.workers else name
        self.name = make_name(interface.identifier.name)

        # self.extendedAttributes is a dict of dicts, keyed on
        # all/getterOnly/setterOnly and then on member name. Values are an
        # array of extended attributes.
        self.extendedAttributes = { 'all': {}, 'getterOnly': {}, 'setterOnly': {} }

        def addExtendedAttribute(attribute, config):
            def add(key, members, attribute):
                for member in members:
                    self.extendedAttributes[key].setdefault(member, []).append(attribute)

            if isinstance(config, dict):
                for key in ['all', 'getterOnly', 'setterOnly']:
                    add(key, config.get(key, []), attribute)
            elif isinstance(config, list):
                add('all', config, attribute)
            else:
                assert isinstance(config, str)
                if config == '*':
                    iface = self.interface
                    while iface:
                        add('all', map(lambda m: m.name, iface.members), attribute)
                        iface = iface.parent
                else:
                    add('all', [config], attribute)

        for attribute in ['implicitJSContext', 'resultNotAddRefed']:
            addExtendedAttribute(attribute, desc.get(attribute, {}))

        self.binaryNames = desc.get('binaryNames', {})

        # Build the prototype chain.
        self.prototypeChain = []
        parent = interface
        while parent:
            self.prototypeChain.insert(0, make_name(parent.identifier.name))
            parent = parent.parent
        config.maxProtoChainLength = max(config.maxProtoChainLength,
                                         len(self.prototypeChain))

    def hasInterfaceOrInterfacePrototypeObject(self):

        # Forward-declared interfaces don't need either interface object or
        # interface prototype object as they're going to use QI (on main thread)
        # or be passed as a JSObject (on worker threads).
        if self.interface.isExternal():
            return False

        return self.interface.hasInterfaceObject() or self.interface.hasInterfacePrototypeObject()

    def getExtendedAttributes(self, member, getter=False, setter=False):
        def ensureValidInfallibilityExtendedAttribute(name, attr):
            assert(attr is None or attr is True or len(attr) == 1)
            if (attr is not None and attr is not True and
                'Workers' not in attr and 'MainThread' not in attr):
                raise TypeError(("Unknown value for '%s': " % name) + attr[0])

        name = member.identifier.name
        if member.isMethod():
            attrs = self.extendedAttributes['all'].get(name, [])
            throws = member.getExtendedAttribute("Throws")
            ensureValidInfallibilityExtendedAttribute("Throws", throws)
            if (throws is None or
                (throws is not True and
                 ('Workers' not in throws or not self.workers) and
                 ('MainThread' not in throws or self.workers))):
                attrs.append("infallible")
            return attrs

        assert member.isAttr()
        assert bool(getter) != bool(setter)
        key = 'getterOnly' if getter else 'setterOnly'
        attrs = self.extendedAttributes['all'].get(name, []) + self.extendedAttributes[key].get(name, [])
        infallible = member.getExtendedAttribute("Infallible")
        if infallible is None:
            infallibleAttr = "GetterInfallible" if getter else "SetterInfallible"
            infallible = member.getExtendedAttribute(infallibleAttr)

        ensureValidInfallibilityExtendedAttribute("Infallible", infallible)
        if (infallible is not None and
            (infallible is True or
             ('Workers' in infallible and self.workers) or
             ('MainThread' in infallible and not self.workers))):
            attrs.append("infallible")

        return attrs
