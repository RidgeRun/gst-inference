include $(top_srcdir)/common/check.mak

CHECK_REGISTRY = $(top_builddir)/tests/check/test-registry.reg
TEST_FILES_DIRECTORY = $(top_srcdir)/tests/files

REGISTRY_ENVIRONMENT = \
	GST_REGISTRY_1_0=$(CHECK_REGISTRY)

AM_TESTS_ENVIRONMENT += \
	$(REGISTRY_ENVIRONMENT)					\
	GST_PLUGIN_SYSTEM_PATH_1_0=				\
	GST_PLUGIN_PATH_1_0=$(top_builddir)/gst:$(top_builddir)/ext:$(top_builddir)/sys:$(GSTPB_PLUGINS_DIR):$(GST_PLUGINS_DIR)			\
	GST_PLUGIN_LOADING_WHITELIST="gstreamer@$(GST_PLUGINS_DIR):gst-plugins-base@$(GSTPB_PLUGINS_DIR):gst-inference@$(top_builddir)"	\
	GST_STATE_IGNORE_ELEMENTS= \
	GSETTINGS_BACKEND="memory"

# the core dumps of some machines have PIDs appended
CLEANFILES = core.* test-registry.*

clean-local: clean-local-check

#if USE_PLUGIN_EXAMPLE
#check_example = \
#	elements/example \
#	elements/example
#else
#check_example =
#endif

check_PROGRAMS =

VALGRIND_TO_FIX =

TESTS = $(check_PROGRAMS)

AM_CFLAGS = $(GST_OBJ_CFLAGS) $(GST_CHECK_CFLAGS) $(CHECK_CFLAGS) \
	$(GST_OPTION_CFLAGS) $(GST_CFLAGS) -DGST_TEST_FILES_PATH="\"$(TEST_FILES_DIRECTORY)\"" \
	-DGST_CHECK_TEST_ENVIRONMENT_BEACON="\"GST_PLUGIN_LOADING_WHITELIST\"" \
	-UG_DISABLE_ASSERT -UG_DISABLE_CAST_CHECKS $(PTHREAD_CFLAGS)
LDADD = $(GST_OBJ_LIBS) $(GST_CHECK_LIBS) $(CHECK_LIBS)

# valgrind testing
VALGRIND_TESTS_DISABLE = \
	$(VALGRIND_TO_FIX)

SUPPRESSIONS = $(top_srcdir)/common/gst.supp
