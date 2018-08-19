/*
  Native File Dialog

  http://www.frogtoss.com/labs

  We do not check for malloc failure on Linux - Linux overcommits memory!
*/

#include <stdio.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <gtk/gtk.h>
#include "nfd.h"


namespace {

    template <typename T>
    struct Free_Guard {
        T* data;
        Free_Guard(T* freeable) noexcept : data(freeable) {}
        ~Free_Guard() {
            NFDi_Free(data);
        }
    };

    template <typename T>
    struct FreeCheck_Guard {
        T* data;
        FreeCheck_Guard(T* freeable = nullptr) noexcept : data(freeable) {}
        ~FreeCheck_Guard() {
            if (data) NFDi_Free(data);
        }
    };

    /* current error */
    const char* g_errorstr = nullptr;

    void NFDi_SetError(const char *msg)
    {
        g_errorstr = msg;
    }

    template <typename T = void>
    T *NFDi_Malloc(size_t bytes)
    {
        void *ptr = malloc(bytes);
        if (!ptr)
            NFDi_SetError("NFDi_Malloc failed.");

        return static_cast<T*>(ptr);
    }

    template <typename T>
    void NFDi_Free(T *ptr)
    {
        assert(ptr);
        free(static_cast<void*>(ptr));
    }

    template <typename T>
    T* copy(const T* begin, const T* end, T* out) {
        for (; begin != end; ++begin) {
            *out++ = *begin;
        }
        return out;
    }
    
    // Does not own the filter and extension.
    struct Pair_GtkFileFilter_FileExtension {
		GtkFileFilter* filter;
		const nfdnchar_t* extensionBegin;
		const nfdnchar_t* extensionEnd;
	};
	
	struct ButtonClickedArgs {
		Pair_GtkFileFilter_FileExtension* map;
		GtkFileChooser* chooser;
	};
	
	
    void AddFiltersToDialog(GtkFileChooser* chooser, const nfdnfilteritem_t* filterList, nfdfiltersize_t filterCount) {
        
        if (filterCount) {
            assert(filterList);

            // we have filters to add ... format and add them

            for (nfdfiltersize_t index = 0; index != filterCount; ++index) {

                GtkFileFilter* filter = gtk_file_filter_new();

                // count number of file extensions
                size_t sep = 1;
                for (const nfdnchar_t *p_spec = filterList[index].spec; *p_spec; ++p_spec) {
                    if (*p_spec == L',') {
                        ++sep;
                    }
                }

                // friendly name conversions: "png,jpg" -> "Image files (png, jpg)"

                // calculate space needed (including the trailing '\0')
                size_t nameSize = sep + strlen(filterList[index].spec) + 3 + strlen(filterList[index].name);
                
                // malloc the required memory
                nfdnchar_t *nameBuf = NFDi_Malloc<nfdnchar_t>(sizeof(nfdnchar_t) * nameSize);

                nfdnchar_t *p_nameBuf = nameBuf;
                for (const nfdnchar_t *p_filterName = filterList[index].name; *p_filterName; ++p_filterName) {
                    *p_nameBuf++ = *p_filterName;
                }
                *p_nameBuf++ = ' ';
                *p_nameBuf++ = '(';
                const nfdnchar_t *p_extensionStart = filterList[index].spec;
                for (const nfdnchar_t *p_spec = filterList[index].spec; true; ++p_spec) {
                    if (*p_spec == ',' || !*p_spec) {
                        if (*p_spec == ',') {
                            *p_nameBuf++ = ',';
                            *p_nameBuf++ = ' ';
                        }

                        // +1 for the trailing '\0'
                        nfdnchar_t *extnBuf = NFDi_Malloc<nfdnchar_t>(sizeof(nfdnchar_t) * (p_spec - p_extensionStart + 3));
                        nfdnchar_t *p_extnBufEnd = extnBuf;
                        *p_extnBufEnd++ = '*';
                        *p_extnBufEnd++ = '.';
                        p_extnBufEnd = copy(p_extensionStart, p_spec, p_extnBufEnd);
                        *p_extnBufEnd++ = '\0';
                        assert(p_extnBufEnd - extnBuf == sizeof(nfdnchar_t) * (p_spec - p_extensionStart + 3));
                        gtk_file_filter_add_pattern(filter, extnBuf);
                        NFDi_Free(extnBuf);

                        if (*p_spec) {
                            // update the extension start point
                            p_extensionStart = p_spec + 1;
                        }
                        else {
                            // reached the '\0' character
                            break;
                        }
                    }
                    else {
                        *p_nameBuf++ = *p_spec;
                    }
                }
                *p_nameBuf++ = ')';
                *p_nameBuf++ = '\0';
                assert(p_nameBuf - nameBuf == sizeof(nfdnchar_t) * nameSize);

                // add to the filter
                gtk_file_filter_set_name(filter, nameBuf);

                // free the memory
                NFDi_Free(nameBuf);

                // add filter to chooser
                gtk_file_chooser_add_filter(chooser, filter);
            }
        }

        /* always append a wildcard option to the end*/

        GtkFileFilter* filter = gtk_file_filter_new();
        gtk_file_filter_set_name(filter, "All files");
        gtk_file_filter_add_pattern(filter, "*");
        gtk_file_chooser_add_filter(chooser, filter);
    }
    
    // returns null-terminated map (trailing .filter is null)
    Pair_GtkFileFilter_FileExtension* AddFiltersToDialogWithMap(GtkFileChooser* chooser, const nfdnfilteritem_t* filterList, nfdfiltersize_t filterCount) {
        
        Pair_GtkFileFilter_FileExtension* map = NFDi_Malloc<Pair_GtkFileFilter_FileExtension>(sizeof(Pair_GtkFileFilter_FileExtension) * (filterCount + 1));
        
        if (filterCount) {
            assert(filterList);

            // we have filters to add ... format and add them

            for (nfdfiltersize_t index = 0; index != filterCount; ++index) {

                GtkFileFilter* filter = gtk_file_filter_new();
                
                // store filter in map
                map[index].filter = filter;
                map[index].extensionBegin = filterList[index].spec;
                map[index].extensionEnd = nullptr;

                // count number of file extensions
                size_t sep = 1;
                for (const nfdnchar_t *p_spec = filterList[index].spec; *p_spec; ++p_spec) {
                    if (*p_spec == L',') {
                        ++sep;
                    }
                }

                // friendly name conversions: "png,jpg" -> "Image files (png, jpg)"

                // calculate space needed (including the trailing '\0')
                size_t nameSize = sep + strlen(filterList[index].spec) + 3 + strlen(filterList[index].name);
                
                // malloc the required memory
                nfdnchar_t *nameBuf = NFDi_Malloc<nfdnchar_t>(sizeof(nfdnchar_t) * nameSize);

                nfdnchar_t *p_nameBuf = nameBuf;
                for (const nfdnchar_t *p_filterName = filterList[index].name; *p_filterName; ++p_filterName) {
                    *p_nameBuf++ = *p_filterName;
                }
                *p_nameBuf++ = ' ';
                *p_nameBuf++ = '(';
                const nfdnchar_t *p_extensionStart = filterList[index].spec;
                for (const nfdnchar_t *p_spec = filterList[index].spec; true; ++p_spec) {
                    if (*p_spec == ',' || !*p_spec) {
                        if (*p_spec == ',') {
                            *p_nameBuf++ = ',';
                            *p_nameBuf++ = ' ';
                        }

                        // +1 for the trailing '\0'
                        nfdnchar_t *extnBuf = NFDi_Malloc<nfdnchar_t>(sizeof(nfdnchar_t) * (p_spec - p_extensionStart + 3));
                        nfdnchar_t *p_extnBufEnd = extnBuf;
                        *p_extnBufEnd++ = '*';
                        *p_extnBufEnd++ = '.';
                        p_extnBufEnd = copy(p_extensionStart, p_spec, p_extnBufEnd);
                        *p_extnBufEnd++ = '\0';
                        assert(p_extnBufEnd - extnBuf == sizeof(nfdnchar_t) * (p_spec - p_extensionStart + 3));
                        gtk_file_filter_add_pattern(filter, extnBuf);
                        NFDi_Free(extnBuf);
                        
                        // store current pointer in map (if it's the first one)
                        if (map[index].extensionEnd == nullptr) {
							map[index].extensionEnd = p_spec;
						}

                        if (*p_spec) {
                            // update the extension start point
                            p_extensionStart = p_spec + 1;
                        }
                        else {
                            // reached the '\0' character
                            break;
                        }
                    }
                    else {
                        *p_nameBuf++ = *p_spec;
                    }
                }
                *p_nameBuf++ = ')';
                *p_nameBuf++ = '\0';
                assert(p_nameBuf - nameBuf == sizeof(nfdnchar_t) * nameSize);
                
                // add to the filter
                gtk_file_filter_set_name(filter, nameBuf);

                // free the memory
                NFDi_Free(nameBuf);

                // add filter to chooser
                gtk_file_chooser_add_filter(chooser, filter);
            }
        }
        // set trailing map index to null
        map[filterCount].filter = nullptr;

        /* always append a wildcard option to the end*/
        GtkFileFilter* filter = gtk_file_filter_new();
        gtk_file_filter_set_name(filter, "All files");
        gtk_file_filter_add_pattern(filter, "*");
        gtk_file_chooser_add_filter(chooser, filter);
        
        return map;
    }

    void SetDefaultPath(GtkFileChooser *chooser, const char *defaultPath)
    {
        if (!defaultPath || !*defaultPath)
            return;

        /* GTK+ manual recommends not specifically setting the default path.
        We do it anyway in order to be consistent across platforms.

        If consistency with the native OS is preferred, this is the line
        to comment out. -ml */
        gtk_file_chooser_set_current_folder(chooser, defaultPath);
    }

    void WaitForCleanup()
    {
        while (gtk_events_pending()) gtk_main_iteration();
    }

    struct Widget_Guard {
        GtkWidget* data;
        Widget_Guard(GtkWidget* widget) : data(widget) {}
        ~Widget_Guard() {
            WaitForCleanup();
            gtk_widget_destroy(data);
            WaitForCleanup();
        }
    };
    
    static void FileActivatedSignalHandler(GtkButton* saveButton, void* userdata)
    {
		ButtonClickedArgs* args = static_cast<ButtonClickedArgs*>(userdata);
		GtkFileChooser* chooser = args->chooser;
		char* currentFileName = gtk_file_chooser_get_current_name(chooser);
		if (*currentFileName) { // string is not empty
			
			// find a '.' in the file name
			const char* p_period = currentFileName;
			for (;*p_period; ++p_period) {
				if (*p_period == '.') {
					break;
				}
			}
			
			if (!*p_period) { // there is no '.', so append the default extension
				Pair_GtkFileFilter_FileExtension* filterMap = static_cast<Pair_GtkFileFilter_FileExtension*>(args->map);
				GtkFileFilter* currentFilter = gtk_file_chooser_get_filter(chooser);
				if (currentFilter) {
					for (;filterMap->filter; ++filterMap) {
						if (filterMap->filter == currentFilter) break;
					}
				}
				if (filterMap->filter) {
					// memory for appended string (including '.' and trailing '\0')
					char* appendedFileName = NFDi_Malloc<char>(sizeof(char) * ((p_period - currentFileName) + (filterMap->extensionEnd - filterMap->extensionBegin) + 2));
					char* p_fileName = copy(currentFileName, p_period, appendedFileName);
					*p_fileName++ = '.';
					p_fileName = copy(filterMap->extensionBegin, filterMap->extensionEnd, p_fileName);
					*p_fileName++ = '\0';
					
					assert(p_fileName - appendedFileName == (p_period - currentFileName) + (filterMap->extensionEnd - filterMap->extensionBegin) + 2);
					
					// set the appended file name
					gtk_file_chooser_set_current_name(chooser, appendedFileName);
					
					// free the memory
					NFDi_Free(appendedFileName);
				}
			}
		}
		
		// free the memory
		g_free(currentFileName);
    }
}

const char *NFD_GetError(void)
{
    return g_errorstr;
}

void NFD_ClearError(void)
{
    NFDi_SetError(nullptr);
}



/* public */

nfdresult_t NFD_Init(void) {
    // Init GTK
    if (!gtk_init_check(NULL, NULL))
    {
        NFDi_SetError("gtk_init_check failed to initilaize GTK+");
        return NFD_ERROR;
    }
    return NFD_OKAY;
}
void NFD_Quit(void) {
    // do nothing, GTK cannot be de-initialized
}

void NFD_FreePathN(nfdnchar_t* filePath) {
    assert(filePath);
    g_free(filePath);
}

nfdresult_t NFD_OpenDialogN( const nfdnfilteritem_t *filterList,
                             nfdfiltersize_t count,
                             const nfdnchar_t *defaultPath,
                             nfdnchar_t **outPath )
{
    GtkWidget* widget = gtk_file_chooser_dialog_new(
        "Open File",
        nullptr,
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        nullptr
    );

    // guard to destroy the widget when returning from this function
    Widget_Guard widgetGuard(widget);

    /* Build the filter list */
    AddFiltersToDialog(GTK_FILE_CHOOSER(widget), filterList, count);

    /* Set the default path */
    SetDefaultPath(GTK_FILE_CHOOSER(widget), defaultPath);

    if (gtk_dialog_run(GTK_DIALOG(widget)) == GTK_RESPONSE_ACCEPT) {
        // write out the file name
        *outPath = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget));

        return NFD_OKAY;
    }
    else {
        return NFD_CANCEL;
    }
}

nfdresult_t NFD_OpenDialogMultipleN( const nfdnfilteritem_t *filterList,
                                     nfdfiltersize_t count,
                                     const nfdnchar_t *defaultPath,
                                     const nfdpathset_t **outPaths )
{
    GtkWidget* widget = gtk_file_chooser_dialog_new(
        "Open Files",
        nullptr,
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        nullptr
    );

    // guard to destroy the widget when returning from this function
    Widget_Guard widgetGuard(widget);

    // set select multiple
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(widget), TRUE);

    /* Build the filter list */
    AddFiltersToDialog(GTK_FILE_CHOOSER(widget), filterList, count);

    /* Set the default path */
    SetDefaultPath(GTK_FILE_CHOOSER(widget), defaultPath);

    if (gtk_dialog_run(GTK_DIALOG(widget)) == GTK_RESPONSE_ACCEPT) {
        // write out the file name
        GSList *fileList = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(widget));

        *outPaths = static_cast<void*>(fileList);
        return NFD_OKAY;
    }
    else {
        return NFD_CANCEL;
    }
}

nfdresult_t NFD_SaveDialogN( const nfdnfilteritem_t *filterList,
                             nfdfiltersize_t count,
                             const nfdnchar_t *defaultPath,
                             nfdnchar_t **outPath )
{
    GtkWidget* widget = gtk_file_chooser_dialog_new(
        "Save File",
        nullptr,
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Cancel", GTK_RESPONSE_CANCEL,
        nullptr
    );

    // guard to destroy the widget when returning from this function
    Widget_Guard widgetGuard(widget);
    
    GtkWidget* saveButton = gtk_dialog_add_button(GTK_DIALOG(widget), "_Save", GTK_RESPONSE_ACCEPT);

    // Prompt on overwrite
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(widget), TRUE);

    /* Build the filter list */
    ButtonClickedArgs buttonClickedArgs;
    buttonClickedArgs.chooser = GTK_FILE_CHOOSER(widget);
    buttonClickedArgs.map = AddFiltersToDialogWithMap(GTK_FILE_CHOOSER(widget), filterList, count);
    
    /* Set the default path */
    SetDefaultPath(GTK_FILE_CHOOSER(widget), defaultPath);
    
    /* set the handler to add file extension */
    gulong handlerID = g_signal_connect(G_OBJECT(saveButton), "pressed", G_CALLBACK(FileActivatedSignalHandler), static_cast<void*>(&buttonClickedArgs));
	
	/* invoke the dialog (blocks until dialog is closed) */
	gint result = gtk_dialog_run(GTK_DIALOG(widget));
	
	/* unset the handler */
	g_signal_handler_disconnect(G_OBJECT(saveButton), handlerID);
	
	/* free the filter map */
	NFDi_Free(buttonClickedArgs.map);
    
    if (result == GTK_RESPONSE_ACCEPT) {
        // write out the file name
        *outPath = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget));

        return NFD_OKAY;
    }
    else {
        return NFD_CANCEL;
    }
}

nfdresult_t NFD_PickFolderN( const nfdnchar_t *defaultPath,
                             nfdnchar_t **outPath )
{
    GtkWidget* widget = gtk_file_chooser_dialog_new(
        "Select folder",
        nullptr,
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Select", GTK_RESPONSE_ACCEPT,
        nullptr
    );

    // guard to destroy the widget when returning from this function
    Widget_Guard widgetGuard(widget);

    /* Set the default path */
    SetDefaultPath(GTK_FILE_CHOOSER(widget), defaultPath);

    if (gtk_dialog_run(GTK_DIALOG(widget)) == GTK_RESPONSE_ACCEPT) {
        // write out the file name
        *outPath = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget));

        return NFD_OKAY;
    }
    else {
        return NFD_CANCEL;
    }
}



nfdresult_t NFD_PathSet_GetCount(const nfdpathset_t *pathSet, nfdpathsetsize_t* count)
{
    assert(pathSet);
    // const_cast because methods on GSList aren't const, but it should act like const to the caller
    GSList *fileList = const_cast<GSList*>(static_cast<const GSList*>(pathSet));

    *count = g_slist_length(fileList);
    return NFD_OKAY;
}

nfdresult_t NFD_PathSet_GetPathN(const nfdpathset_t *pathSet, nfdpathsetsize_t index, nfdnchar_t **outPath)
{
    assert(pathSet);
    // const_cast because methods on GSList aren't const, but it should act like const to the caller
    GSList *fileList = const_cast<GSList*>(static_cast<const GSList*>(pathSet));

    // Note: this takes linear time... but should be good enough
    *outPath = static_cast<nfdnchar_t*>(g_slist_nth_data(fileList, index));

    return NFD_OKAY;
}

void NFD_PathSet_FreePathN(const nfdnchar_t* filePath) {
    assert(filePath);
    // no-op, because NFD_PathSet_Free does the freeing for us
}

void NFD_PathSet_Free(const nfdpathset_t *pathSet)
{
    assert(pathSet);
    // const_cast because methods on GSList aren't const, but it should act like const to the caller
    GSList *fileList = const_cast<GSList*>(static_cast<const GSList*>(pathSet));

    // free all the nodes
    for (GSList *node = fileList; node; node = node->next) {
        assert(node->data);
        g_free(node->data);
    }

    // free the path set memory
    g_slist_free(fileList);
}