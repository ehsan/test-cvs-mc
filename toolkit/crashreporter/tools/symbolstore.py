#!/bin/env python
# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is mozilla.org code.
#
# The Initial Developer of the Original Code is
# The Mozilla Foundation
# Portions created by the Initial Developer are Copyright (C) 2007
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
# Ted Mielczarek <ted.mielczarek@gmail.com>
# Ben Turner <mozilla@songbirdnest.com>
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****
#
# Usage: symbolstore.py <params> <dump_syms path> <symbol store path>
#                                <debug info files or dirs>
#   Runs dump_syms on each debug info file specified on the command line,
#   then places the resulting symbol file in the proper directory
#   structure in the symbol store path.  Accepts multiple files
#   on the command line, so can be called as part of a pipe using
#   find <dir> | xargs symbolstore.pl <dump_syms> <storepath>
#   But really, you might just want to pass it <dir>.
#
#   Parameters accepted:
#     -c           : Copy debug info files to the same directory structure
#                    as sym files
#     -a "<archs>" : Run dump_syms -a <arch> for each space separated
#                    cpu architecture in <archs> (only on OS X)
#     -s <srcdir>  : Use <srcdir> as the top source directory to
#                    generate relative filenames.

import sys
import os
import re
import shutil
from optparse import OptionParser

# Utility classes

class VCSFileInfo:
    """ A base class for version-controlled file information. Ensures that the
        following attributes are generated only once (successfully):

            self.root
            self.clean_root
            self.revision
            self.filename

        The attributes are generated by a single call to the GetRoot,
        GetRevision, and GetFilename methods. Those methods are explicitly not
        implemented here and must be implemented in derived classes. """

    def __init__(self, file):
        if not file:
            raise ValueError
        self.file = file

    def __getattr__(self, name):
        """ __getattr__ is only called for attributes that are not set on self,
            so setting self.[attr] will prevent future calls to the GetRoot,
            GetRevision, and GetFilename methods. We don't set the values on
            failure on the off chance that a future call might succeed. """

        if name == "root":
            root = self.GetRoot()
            if root:
                self.root = root
            return root

        elif name == "clean_root":
            clean_root = self.GetCleanRoot()
            if clean_root:
                self.clean_root = clean_root
            return clean_root

        elif name == "revision":
            revision = self.GetRevision()
            if revision:
                self.revision = revision
            return revision

        elif name == "filename":
            filename = self.GetFilename()
            if filename:
                self.filename = filename
            return filename

        raise AttributeError

    def GetRoot(self):
        """ This method should return the unmodified root for the file or 'None'
            on failure. """
        raise NotImplementedError

    def GetCleanRoot(self):
        """ This method should return the repository root for the file or 'None'
            on failure. """
        raise NotImplementedErrors

    def GetRevision(self):
        """ This method should return the revision number for the file or 'None'
            on failure. """
        raise NotImplementedError

    def GetFilename(self):
        """ This method should return the repository-specific filename for the
            file or 'None' on failure. """
        raise NotImplementedError

class CVSFileInfo(VCSFileInfo):
    """ A class to maintiain version information for files in a CVS repository.
        Derived from VCSFileInfo. """

    def __init__(self, file, srcdir):
        VCSFileInfo.__init__(self, file)
        self.srcdir = srcdir

    def GetRoot(self):
        (path, filename) = os.path.split(self.file)
        root = os.path.join(path, "CVS", "Root")
        if not os.path.isfile(root):
            return None
        f = open(root, "r")
        root_name = f.readline().strip()
        f.close()
        if root_name:
            return root_name
        print >> sys.stderr, "Failed to get CVS Root for %s" % filename
        return None

    def GetCleanRoot(self):
        parts = self.root.split('@')
        if len(parts) > 1:
            # we don't want the extra colon
            return parts[1].replace(":","")
        return self.root.replace(":","")

    def GetRevision(self):
        (path, filename) = os.path.split(self.file)
        entries = os.path.join(path, "CVS", "Entries")
        if not os.path.isfile(entries):
            return None
        f = open(entries, "r")
        for line in f:
            parts = line.split("/")
            if len(parts) > 1 and parts[1] == filename:
                return parts[2]
        print >> sys.stderr, "Failed to get CVS Revision for %s" % filename
        return None

    def GetFilename(self):
        file = self.file
        if self.revision and self.clean_root:
            if self.srcdir:
                # strip the base path off
                # but we actually want the last dir in srcdir
                file = os.path.normpath(file)
                # the lower() is to handle win32+vc8, where
                # the source filenames come out all lowercase,
                # but the srcdir can be mixed case
                if file.lower().startswith(self.srcdir.lower()):
                    file = file[len(self.srcdir):]
                (head, tail) = os.path.split(self.srcdir)
                if tail == "":
                    tail = os.path.basename(head)
                file = tail + file
            return "cvs:%s:%s:%s" % (self.clean_root, file, self.revision)
        return file

# This regex separates protocol and optional username/password from a url.
# For instance, all the following urls will be transformed into
# 'foo.com/bar':
#
#   http://foo.com/bar
#   svn+ssh://user@foo.com/bar
#   svn+ssh://user:pass@foo.com/bar
#
# This is used by both SVN and HG
rootRegex = re.compile(r'^\S+?:/+(?:[^\s/]*@)?(\S+)$')

class SVNFileInfo(VCSFileInfo):
    url = None
    repo = None
    svndata = {}

    def __init__(self, file):
        """ We only want to run subversion's info tool once so pull all the data
            here. """

        VCSFileInfo.__init__(self, file)

        if os.path.isfile(file):
            command = os.popen("svn info %s" % file, "r")
            for line in command:
                # The last line of the output is usually '\n'
                if line.strip() == '':
                    continue
                # Split into a key/value pair on the first colon
                key, value = line.split(':', 1)
                if key in ["Repository Root", "Revision", "URL"]:
                    self.svndata[key] = value.strip()

            exitStatus = command.close()
            if exitStatus:
              print >> sys.stderr, "Failed to get SVN info for %s" % file

    def GetRoot(self):
        key = "Repository Root"
        if key in self.svndata:
            match = rootRegex.match(self.svndata[key])
            if match:
                return match.group(1)
        print >> sys.stderr, "Failed to get SVN Root for %s" % self.file
        return None

    # File bug to get this teased out from the current GetRoot, this is temporary
    def GetCleanRoot(self):
        return self.root

    def GetRevision(self):
        key = "Revision"
        if key in self.svndata:
            return self.svndata[key]
        print >> sys.stderr, "Failed to get SVN Revision for %s" % self.file
        return None

    def GetFilename(self):
        if self.root and self.revision:
            if "URL" in self.svndata and "Repository Root" in self.svndata:
                url, repo = self.svndata["URL"], self.svndata["Repository Root"]
                file = url[len(repo) + 1:]
            return "svn:%s:%s:%s" % (self.root, file, self.revision)
        print >> sys.stderr, "Failed to get SVN Filename for %s" % self.file
        return self.file

class HGRepoInfo():
    # HG info is per-repo, so cache it in a static
    # member var
    repos = {}
    def __init__(self, path, rev, cleanroot):
        self.path = path
        self.rev = rev
        self.cleanroot = cleanroot

class HGFileInfo(VCSFileInfo):
    def __init__(self, file, srcdir):
        VCSFileInfo.__init__(self, file)
        # we should only have to collect this info once per-repo
        if not srcdir in HGRepoInfo.repos:
            rev = os.popen('hg identify -i "%s"' % srcdir, "r").readlines()[0].rstrip()
            # could have a + if there are uncommitted local changes
            if rev.endswith('+'):
                rev = rev[:-1]

            path = os.popen('hg -R "%s" showconfig paths.default' % srcdir, "r").readlines()[0].rstrip()
            if path == '':
                hg_root = os.environ.get("SRCSRV_ROOT")
                if hg_root:
                    path = hg_root
                else:
                    print >> sys.stderr, "Failed to get HG Repo for %s" % srcdir
            if path != '': # not there?
                match = rootRegex.match(path)
                if match:
                    cleanroot = match.group(1)
                    if cleanroot.endswith('/'):
                        cleanroot = cleanroot[:-1]
            HGRepoInfo.repos[srcdir] = HGRepoInfo(path, rev, cleanroot)
        self.repo = HGRepoInfo.repos[srcdir]
        self.file = file
        self.srcdir = srcdir

    def GetRoot(self):
        return self.repo.path

    def GetCleanRoot(self):
        return self.repo.cleanroot

    def GetRevision(self):
        return self.repo.rev

    def GetFilename(self):
        file = self.file
        if self.revision and self.clean_root:
            if self.srcdir:
                # strip the base path off
                file = os.path.normpath(file)
                if IsInDir(file, self.srcdir):
                    file = file[len(self.srcdir):]
                if file.startswith('/') or file.startswith('\\'):
                    file = file[1:]
            return "hg:%s:%s:%s" % (self.clean_root, file, self.revision)
        return file

# Utility functions

# A cache of files for which VCS info has already been determined. Used to
# prevent extra filesystem activity or process launching.
vcsFileInfoCache = {}

def IsInDir(file, dir):
    # the lower() is to handle win32+vc8, where
    # the source filenames come out all lowercase,
    # but the srcdir can be mixed case
    return os.path.abspath(file).lower().startswith(os.path.abspath(dir).lower())

def GetVCSFilename(file, srcdir):
    """Given a full path to a file, and the top source directory,
    look for version control information about this file, and return
    a tuple containing
    1) a specially formatted filename that contains the VCS type,
    VCS location, relative filename, and revision number, formatted like:
    vcs:vcs location:filename:revision
    For example:
    cvs:cvs.mozilla.org/cvsroot:mozilla/browser/app/nsBrowserApp.cpp:1.36
    2) the unmodified root information if it exists"""
    (path, filename) = os.path.split(file)
    if path == '' or filename == '':
        return (file, None)

    fileInfo = None
    root = ''
    if file in vcsFileInfoCache:
        # Already cached this info, use it.
        fileInfo = vcsFileInfoCache[file]
    else:
        if os.path.isdir(os.path.join(path, "CVS")):
            fileInfo = CVSFileInfo(file, srcdir)
        elif os.path.isdir(os.path.join(path, ".svn")) or \
             os.path.isdir(os.path.join(path, "_svn")):
            fileInfo = SVNFileInfo(file);
        elif os.path.isdir(os.path.join(srcdir, '.hg')) and \
             IsInDir(file, srcdir):
            fileInfo = HGFileInfo(file, srcdir)
        vcsFileInfoCache[file] = fileInfo

    if fileInfo:
        file = fileInfo.filename
        root = fileInfo.root

    # we want forward slashes on win32 paths
    return (file.replace("\\", "/"), root)

def GetPlatformSpecificDumper(**kwargs):
    """This function simply returns a instance of a subclass of Dumper
    that is appropriate for the current platform."""
    return {'win32': Dumper_Win32,
            'cygwin': Dumper_Win32,
            'linux2': Dumper_Linux,
            'sunos5': Dumper_Solaris,
            'darwin': Dumper_Mac}[sys.platform](**kwargs)

def SourceIndex(fileStream, outputPath, vcs_root):
    """Takes a list of files, writes info to a data block in a .stream file"""
    # Creates a .pdb.stream file in the mozilla\objdir to be used for source indexing
    # Create the srcsrv data block that indexes the pdb file
    result = True
    pdbStreamFile = open(outputPath, "w")
    pdbStreamFile.write('''SRCSRV: ini ------------------------------------------------\r\nVERSION=2\r\nINDEXVERSION=2\r\nVERCTRL=http\r\nSRCSRV: variables ------------------------------------------\r\nHGSERVER=''')
    pdbStreamFile.write(vcs_root)
    pdbStreamFile.write('''\r\nSRCSRVVERCTRL=http\r\nHTTP_EXTRACT_TARGET=%hgserver%/raw-file/%var3%/%var2%\r\nSRCSRVTRG=%http_extract_target%\r\nSRCSRV: source files ---------------------------------------\r\n''')
    pdbStreamFile.write(fileStream) # can't do string interpolation because the source server also uses this and so there are % in the above
    pdbStreamFile.write("SRCSRV: end ------------------------------------------------\r\n\n")
    pdbStreamFile.close()
    return result

class Dumper:
    """This class can dump symbols from a file with debug info, and
    store the output in a directory structure that is valid for use as
    a Breakpad symbol server.  Requires a path to a dump_syms binary--
    |dump_syms| and a directory to store symbols in--|symbol_path|.
    Optionally takes a list of processor architectures to process from
    each debug file--|archs|, the full path to the top source
    directory--|srcdir|, for generating relative source file names,
    and an option to copy debug info files alongside the dumped
    symbol files--|copy_debug|, mostly useful for creating a
    Microsoft Symbol Server from the resulting output.

    You don't want to use this directly if you intend to call
    ProcessDir.  Instead, call GetPlatformSpecificDumper to
    get an instance of a subclass."""
    def __init__(self, dump_syms, symbol_path,
                 archs=None, srcdir=None, copy_debug=False, vcsinfo=False, srcsrv=False):
        # popen likes absolute paths, at least on windows
        self.dump_syms = os.path.abspath(dump_syms)
        self.symbol_path = symbol_path
        if archs is None:
            # makes the loop logic simpler
            self.archs = ['']
        else:
            self.archs = ['-a %s' % a for a in archs.split()]
        if srcdir is not None:
            self.srcdir = os.path.normpath(srcdir)
        else:
            self.srcdir = None
        self.copy_debug = copy_debug
        self.vcsinfo = vcsinfo
        self.srcsrv = srcsrv

    # subclasses override this
    def ShouldProcess(self, file):
        return False

    # and can override this
    def ShouldSkipDir(self, dir):
        return False

    def RunFileCommand(self, file):
        """Utility function, returns the output of file(1)"""
        try:
            # we use -L to read the targets of symlinks,
            # and -b to print just the content, not the filename
            return os.popen("file -Lb " + file).read()
        except:
            return ""

    # This is a no-op except on Win32
    def FixFilenameCase(self, file):
        return file

    # This is a no-op except on Win32
    def SourceServerIndexing(self, debug_file, guid, sourceFileStream, vcs_root):
        return ""

    # subclasses override this if they want to support this
    def CopyDebug(self, file, debug_file, guid):
        pass

    def Process(self, file_or_dir):
        "Process a file or all the (valid) files in a directory."
        if os.path.isdir(file_or_dir) and not self.ShouldSkipDir(file_or_dir):
            return self.ProcessDir(file_or_dir)
        elif os.path.isfile(file_or_dir):
            return self.ProcessFile(file_or_dir)
        # maybe it doesn't exist?
        return False

    def ProcessDir(self, dir):
        """Process all the valid files in this directory.  Valid files
        are determined by calling ShouldProcess."""
        result = True
        for root, dirs, files in os.walk(dir):
            for d in dirs[:]:
                if self.ShouldSkipDir(d):
                    dirs.remove(d)
            for f in files:
                fullpath = os.path.join(root, f)
                if self.ShouldProcess(fullpath):
                    if not self.ProcessFile(fullpath):
                        result = False
        return result

    def ProcessFile(self, file):
        """Dump symbols from this file into a symbol file, stored
        in the proper directory structure in  |symbol_path|."""
        result = False
        sourceFileStream = ''
        # tries to get the vcs root from the .mozconfig first - if it's not set
        # the tinderbox vcs path will be assigned further down
        vcs_root = os.environ.get("SRCSRV_ROOT")
        for arch in self.archs:
            try:
                cmd = os.popen("%s %s %s" % (self.dump_syms, arch, file), "r")
                module_line = cmd.next()
                if module_line.startswith("MODULE"):
                    # MODULE os cpu guid debug_file
                    (guid, debug_file) = (module_line.split())[3:5]
                    # strip off .pdb extensions, and append .sym
                    sym_file = re.sub("\.pdb$", "", debug_file) + ".sym"
                    # we do want forward slashes here
                    rel_path = os.path.join(debug_file,
                                            guid,
                                            sym_file).replace("\\", "/")
                    full_path = os.path.normpath(os.path.join(self.symbol_path,
                                                              rel_path))
                    try:
                        os.makedirs(os.path.dirname(full_path))
                    except OSError: # already exists
                        pass
                    f = open(full_path, "w")
                    f.write(module_line)
                    # now process the rest of the output
                    for line in cmd:
                        if line.startswith("FILE"):
                            # FILE index filename
                            (x, index, filename) = line.split(None, 2)
                            if sys.platform == "sunos5":
                                start = filename.find(self.srcdir)
                                if start == -1:
                                    start = 0
                                filename = filename[start:]
                            filename = self.FixFilenameCase(filename.rstrip())
                            sourcepath = filename
                            if self.vcsinfo:
                                (filename, rootname) = GetVCSFilename(filename, self.srcdir)
                                # sets vcs_root in case the loop through files were to end on an empty rootname
                                if vcs_root is None:
                                  if rootname:
                                     vcs_root = rootname
                            # gather up files with hg for indexing   
                            if filename.startswith("hg"):
                                (ver, checkout, source_file, revision) = filename.split(":", 3)
                                sourceFileStream += sourcepath + "*" + source_file + '*' + revision + "\r\n"
                            f.write("FILE %s %s\n" % (index, filename))
                        else:
                            # pass through all other lines unchanged
                            f.write(line)
                    f.close()
                    cmd.close()
                    # we output relative paths so callers can get a list of what
                    # was generated
                    print rel_path
                    if self.copy_debug:
                        self.CopyDebug(file, debug_file, guid)
                    if self.srcsrv and vcs_root:
                        # Call on SourceServerIndexing
                        result = self.SourceServerIndexing(debug_file, guid, sourceFileStream, vcs_root)
                    result = True
            except StopIteration:
                pass
            except:
                print >> sys.stderr, "Unexpected error: ", sys.exc_info()[0]
                raise
        return result

# Platform-specific subclasses.  For the most part, these just have
# logic to determine what files to extract symbols from.

class Dumper_Win32(Dumper):
    fixedFilenameCaseCache = {}

    def ShouldProcess(self, file):
        """This function will allow processing of pdb files that have dll
        or exe files with the same base name next to them."""
        if file.endswith(".pdb"):
            (path,ext) = os.path.splitext(file)
            if os.path.isfile(path + ".exe") or os.path.isfile(path + ".dll"):
                return True
        return False

    def FixFilenameCase(self, file):
        """Recent versions of Visual C++ put filenames into
        PDB files as all lowercase.  If the file exists
        on the local filesystem, fix it."""

        # Use a cached version if we have one.
        if file in self.fixedFilenameCaseCache:
            return self.fixedFilenameCaseCache[file]

        result = file

        (path, filename) = os.path.split(file)
        if os.path.isdir(path):
            lc_filename = filename.lower()
            for f in os.listdir(path):
                if f.lower() == lc_filename:
                    result = os.path.join(path, f)
                    break

        # Cache the corrected version to avoid future filesystem hits.
        self.fixedFilenameCaseCache[file] = result
        return result

    def CopyDebug(self, file, debug_file, guid):
        rel_path = os.path.join(debug_file,
                                guid,
                                debug_file).replace("\\", "/")
        print rel_path
        full_path = os.path.normpath(os.path.join(self.symbol_path,
                                                  rel_path))
        shutil.copyfile(file, full_path)
        pass
        
    def SourceServerIndexing(self, debug_file, guid, sourceFileStream, vcs_root):
        # Creates a .pdb.stream file in the mozilla\objdir to be used for source indexing
        cwd = os.getcwd()
        streamFilename = debug_file + ".stream"
        stream_output_path = os.path.join(cwd, streamFilename)
        # Call SourceIndex to create the .stream file
        result = SourceIndex(sourceFileStream, stream_output_path, vcs_root)
        
        if self.copy_debug:
            pdbstr_path = os.environ.get("PDBSTR_PATH")
            pdbstr = os.path.normpath(pdbstr_path)
            pdb_rel_path = os.path.join(debug_file, guid, debug_file)
            pdb_filename = os.path.normpath(os.path.join(self.symbol_path, pdb_rel_path))
            # move to the dir with the stream files to call pdbstr
            os.chdir(os.path.dirname(stream_output_path))
            os.spawnv(os.P_WAIT, pdbstr, [pdbstr, "-w", "-p:" + pdb_filename, "-i:" + streamFilename, "-s:srcsrv"])
            # clean up all the .stream files when done
            os.remove(stream_output_path)
        return result

class Dumper_Linux(Dumper):
    def ShouldProcess(self, file):
        """This function will allow processing of files that are
        executable, or end with the .so extension, and additionally
        file(1) reports as being ELF files.  It expects to find the file
        command in PATH."""
        if file.endswith(".so") or os.access(file, os.X_OK):
            return self.RunFileCommand(file).startswith("ELF")
        return False

    def CopyDebug(self, file, debug_file, guid):
        # We want to strip out the debug info, and add a
        # .gnu_debuglink section to the object, so the debugger can
        # actually load our debug info later.
        file_dbg = file + ".dbg"
        os.system("objcopy --only-keep-debug %s %s" % (file, file_dbg))
        os.system("objcopy --add-gnu-debuglink=%s %s" % (file_dbg, file))
        
        rel_path = os.path.join(debug_file,
                                guid,
                                debug_file + ".dbg")
        full_path = os.path.normpath(os.path.join(self.symbol_path,
                                                  rel_path))
        shutil.copyfile(file_dbg, full_path)
        # gzip the shipped debug files
        os.system("gzip %s" % full_path)
        print rel_path + ".gz"

class Dumper_Solaris(Dumper):
    def RunFileCommand(self, file):
        """Utility function, returns the output of file(1)"""
        try:
            output = os.popen("file " + file).read()
            return output.split('\t')[1];
        except:
            return ""

    def ShouldProcess(self, file):
        """This function will allow processing of files that are
        executable, or end with the .so extension, and additionally
        file(1) reports as being ELF files.  It expects to find the file
        command in PATH."""
        if file.endswith(".so") or os.access(file, os.X_OK):
            return self.RunFileCommand(file).startswith("ELF")
        return False

class Dumper_Mac(Dumper):
    def ShouldProcess(self, file):
        """This function will allow processing of files that are
        executable, or end with the .dylib extension, and additionally
        file(1) reports as being Mach-O files.  It expects to find the file
        command in PATH."""
        if file.endswith(".dylib") or os.access(file, os.X_OK):
            return self.RunFileCommand(file).startswith("Mach-O")
        return False

    def ShouldSkipDir(self, dir):
        """We create .dSYM bundles on the fly, but if someone runs
        buildsymbols twice, we should skip any bundles we created
        previously, otherwise we'll recurse into them and try to 
        dump the inner bits again."""
        if dir.endswith(".dSYM"):
            return True
        return False

    def ProcessFile(self, file):
        """dump_syms on Mac needs to be run on a dSYM bundle produced
        by dsymutil(1), so run dsymutil here and pass the bundle name
        down to the superclass method instead."""
        dsymbundle = file + ".dSYM"
        if os.path.exists(dsymbundle):
            shutil.rmtree(dsymbundle)
        # dsymutil takes --arch=foo instead of -a foo like everything else
        os.system("dsymutil %s %s >/dev/null" % (' '.join([a.replace('-a ', '--arch=') for a in self.archs]),
                                      file))
        return Dumper.ProcessFile(self, dsymbundle)

# Entry point if called as a standalone program
def main():
    parser = OptionParser(usage="usage: %prog [options] <dump_syms binary> <symbol store path> <debug info files>")
    parser.add_option("-c", "--copy",
                      action="store_true", dest="copy_debug", default=False,
                      help="Copy debug info files into the same directory structure as symbol files")
    parser.add_option("-a", "--archs",
                      action="store", dest="archs",
                      help="Run dump_syms -a <arch> for each space separated cpu architecture in ARCHS (only on OS X)")
    parser.add_option("-s", "--srcdir",
                      action="store", dest="srcdir",
                      help="Use SRCDIR to determine relative paths to source files")
    parser.add_option("-v", "--vcs-info",
                      action="store_true", dest="vcsinfo",
                      help="Try to retrieve VCS info for each FILE listed in the output")
    parser.add_option("-i", "--source-index",
                      action="store_true", dest="srcsrv", default=False,
                      help="Add source index information to debug files, making them suitable for use in a source server.")
    (options, args) = parser.parse_args()
    
    #check to see if the pdbstr.exe exists
    if options.srcsrv:
        pdbstr = os.environ.get("PDBSTR_PATH")
        if not os.path.exists(pdbstr):
            print >> sys.stderr, "Invalid path to pdbstr.exe - please set/check PDBSTR_PATH.\n"
            sys.exit(1)
            
    if len(args) < 3:
        parser.error("not enough arguments")
        exit(1)

    dumper = GetPlatformSpecificDumper(dump_syms=args[0],
                                       symbol_path=args[1],
                                       copy_debug=options.copy_debug,
                                       archs=options.archs,
                                       srcdir=options.srcdir,
                                       vcsinfo=options.vcsinfo,
                                       srcsrv=options.srcsrv)
    for arg in args[2:]:
        dumper.Process(arg)

# run main if run directly
if __name__ == "__main__":
    main()
