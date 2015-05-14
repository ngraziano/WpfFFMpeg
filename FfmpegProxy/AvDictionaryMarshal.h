#pragma once

#include <msclr/marshal.h>

namespace msclr
{
	namespace interop
	{
		using namespace System;
		using namespace System::Collections::Generic;

		template <>
			inline IDictionary<String ^, String ^> ^ marshal_as<IDictionary<String ^, String ^> ^, FFMpeg::AVDictionary*>(FFMpeg::AVDictionary* const& from)
		{
			auto result = gcnew Dictionary<String ^, String ^>();

			FFMpeg::AVDictionaryEntry* entry = nullptr;

			while (entry = av_dict_get(from, "", entry, AV_DICT_IGNORE_SUFFIX))
			{
				result->Add(marshal_as<String ^>(entry->key), marshal_as<String ^>(entry->value));
			}
			return result;
		}

		template <>
		ref class context_node<FFMpeg::AVDictionary*, IDictionary<String ^, String ^> ^> : public context_node_base
		{
		private:
			FFMpeg::AVDictionary* toPtr;

		public:
			context_node(FFMpeg::AVDictionary*& toObject, IDictionary<String ^, String ^> ^ fromObject)
			{
				toPtr = nullptr;

				// Context  local,  char* are copied by av_dict_set
				marshal_context context;

				pin_ptr<FFMpeg::AVDictionary*> dictionaryPtr = &toPtr;
				
				for each(System::Collections::Generic::KeyValuePair<String ^, String ^> entry in fromObject)
				{
					const char* key = context.marshal_as<const char*>(entry.Key);
					const char* value = context.marshal_as<const char*>(entry.Value);
					av_dict_set(dictionaryPtr, key, value, 0);
				}
				toObject = toPtr;
			}
			~context_node()
			{
				this->!context_node();
			}

		protected:
			!context_node()
			{
				if (toPtr != nullptr)
				{
					pin_ptr<FFMpeg::AVDictionary*> dictionaryPtr = &toPtr;
					av_dict_free(dictionaryPtr);
					toPtr = nullptr;
				}
			}
		};
	}
}