include $(APPDIR)/Make.defs

PROGNAME  = knowledge_cards
PRIORITY  = $(CONFIG_KNOWLEDGE_CARDS_PRIORITY)
STACKSIZE = $(CONFIG_KNOWLEDGE_CARDS_STACKSIZE)
MODULE    = $(CONFIG_KNOWLEDGE_CARDS_DEMO)
MAINSRC   = src/knowledge_cards_main.c
CSRCS     = src/kc_proxy_stub.c src/kc_voice.c src/kc_deck.c
CSRCS    += src/kc_font_cjk_16.c src/kc_font_20.c src/kc_font_24.c
CSRCS    += $(APPDIR)/packages/ai_agent/src/voice/audio_capture.c
CSRCS    += $(APPDIR)/packages/ai_agent/src/voice/voice_asr.c
CSRCS    += $(APPDIR)/packages/ai_agent/src/voice/volc_asr.c
CSRCS    += $(APPDIR)/packages/ai_agent/src/infra/config_store.c

CFLAGS   += ${INCDIR_PREFIX}$(APPDIR)/packages/ai_agent/include
CFLAGS   += ${INCDIR_PREFIX}$(APPDIR)/packages/ai_agent/src
CFLAGS   += ${INCDIR_PREFIX}$(APPDIR)/netutils/cjson/cJSON
CFLAGS   += ${INCDIR_PREFIX}$(APPDIR)/crypto/mbedtls/mbedtls/include

include $(APPDIR)/Application.mk
