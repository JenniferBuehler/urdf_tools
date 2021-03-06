/**
 * <ORGANIZATION> = Jennifer Buehler 
 * <COPYRIGHT HOLDER> = Jennifer Buehler 
 * 
 * Copyright (c) 2016 Jennifer Buehler 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <ORGANIZATION> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ------------------------------------------------------------------------------
 **/
#ifndef URDF2INVENTOR_HELPERS_H
#define URDF2INVENTOR_HELPERS_H
// Copyright Jennifer Buehler

/**
 * Helper functions for i/o operations.
 * \author Jennifer Buehler
 * \date last edited October 2015
 */

#include <iostream>
#include <string>

#include <urdf_traverser/Helpers.h>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <urdf/model.h>

namespace urdf2inventor
{

namespace helpers
{

/**
 * ################################################################################################################
 * Minor helpers (e.g. file writing)
 * \author Jennifer Buehler
 * \date last edited October 2015
 * ################################################################################################################
 */

extern void resetStdOut();

// see http://homepage.ntlworld.com/jonathan.deboynepollard/FGA/redirecting-standard-io.html
extern void redirectStdOut(const char * toFile);

/**
 * Copies all files as given in \e files to the target directory \e outputDir.
 * Will copy all files i ``<mapIterator->second[i]>`` to ``<output-dir>/<mapIterator->first>``.
 *
 * \param files The file names to copy to target directories:
 *      Map **key** is the path to the file.
 *      It may be an absolute path, in which case \e outputDir is ignored. Or it may
 *      be a relative path, in which case the file is saved relative to \e outputDir.
 *      Map **value** is a list of *absolute* filenames of files to copy into this directory.
 */
extern bool writeFiles(const std::map<std::string, std::set<std::string> >& files, const std::string& outputDir);

/**
 *  All file references in the model \e modelString (to be saved in \e modelDir)
 *  will be changed from absolute paths to references relative to \e modelDir. References
 *  will refer to files which will be stored in \e fileDir or a subdirectory of it.
 *  The directory structure to be created in \e fileDir mirrors the one where the original
 *  files are currently stored, relative to \e fileRootDir.
 *  The directory structure to create in \e fileDir will be returned as part of the result in \e filesToCopy.
 *
 *  Example: \e modelDir is ``iv/myrobot/`` and \e fileDir is ``iv/textures/``. The \e fileRootDir is set to
 *  be ``/home/user/mytextures/``, which has directories ``colortex/`` and ``imagetex/`` in it, both containing
 *  a number of texture files.
 *  Then, assuming the model described in \e modelString will be saved in a file in
 *  ``iv/myrobot/<filename>.<extension>``, all references in \e modelString will point
 *  to ``../textures/colortex/<rest-of-path-to-texture>`` and ``../files/imagetex/<rest-of-path-to-texture>``.
 *
 * \param modelDir when writing the model files to disk, and the installation destination is any ``<install-prefix>``,
 *      all model files will be put in ``<install-prefix>/modelDir``. The path may also be absolute but it's not required.
 * \param fileDir when writing the model files to disk, and the installation destination is any ``<install-prefix>``,
 *      all files will be put in ``<install-prefix>/fileDir``. The path may also be absolute but it's not required.
 * \param fileRootDir the common parent path of all \e filesInUse to use. This can
 *              be the direct result of urdf_traverser::helpers::getCommonParentPath(filesInuse, fileRootDir),
 *              or it can be a directory higher up in the hierarchy, if desired. Based on this path, a directory structure
 *              in the target directory \e fileDir is created which is equal to the directory structure starting at \e fileRootDir
 *              (including *only* directories which contain referenced files).
 *              If the path is not a common parent path of all files in \e filesInUse, the method returns false.
 * \param filesInUse all files which are referenced from within the model. This should be absolute paths!
 * \param modelString the string representation (eg. XML) of the model which is to be adjusted (fixing file references).
 * \param filesToCopy The file names to copy to target directories:
 *      Map **key** is the path to the output file directory, which will be located in
 *      the global output directory (eg. ``<fileDir>/path/to/file``). So if \e fileDir was given as absolute path, this path
 *      will be absolute too, otherwise it will be relative.
 *      Map **value** is a list of *absolute* filenames to copy into this directory.
 *      So copying *all* files i ``<mapIterator->second[i]>`` to ``<output-dir>/<mapIterator->first>`` will be
 *      required when installing the model file \e modelString to ``<output-dir>/<filename>.<extension>``.
 * \return false if no common parent directory could be determined for all of the files and/or the common parent is
 *      no subdirectory of \e fileRootDir.
 */
extern bool fixFileReferences(const std::string& modelDir,
                              const std::string& fileDir,
                              const std::string& fileRootDir,
                              const std::set<std::string>& filesInUse,
                              std::string& modelString,
                              std::map<std::string, std::set<std::string> >& filesToCopy);




}  //  namespace helpers
}  //  namespace urdf2inventor

#endif  // URDF2INVENTOR_HELPERS_H
