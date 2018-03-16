/*
 * local-conference-list-event-handler.cpp
 * Copyright (C) 2010-2018 Belledonne Communications SARL
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "linphone/utils/utils.h"

#include "address/address.h"
#include "content/content.h"
#include "content/content-manager.h"
#include "content/content-type.h"
#include "local-conference-event-handler.h"
#include "local-conference-list-event-handler.h"
#include "xml/resource-lists.h"
#include "xml/rlmi.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

namespace {
	constexpr const char MultipartBoundary[] = "---------------------------14737809831412343453453";
}

// -----------------------------------------------------------------------------

void LocalConferenceListEventHandler::subscribeReceived (const string &xmlBody) {
	list<Content> contents;
	Content rlmiContent;
	rlmiContent.setContentType(ContentType::Rlmi);

	istringstream data(xmlBody);
	unique_ptr<Xsd::ResourceLists::ResourceLists> rl(Xsd::ResourceLists::parseResourceLists(
		data,
		Xsd::XmlSchema::Flags::dont_validate
	));
	for (const auto &l : rl->getList()) {
		for (const auto &entry : l.getEntry()) {
			Address addr(entry.getUri());
			string notifyIdStr = addr.getUriParamValue("Last-Notify");
			addr.removeUriParam("Last-Notify");
			int notifyId = notifyIdStr.empty() ? 0 : Utils::stoi(notifyIdStr);
			ChatRoomId chatRoomId(addr, addr);
			shared_ptr<LocalConferenceEventHandler> handler = findHandler(chatRoomId);
			if (!handler)
				continue;

			string notifyBody = handler->getNotifyForId(notifyId);
			if (notifyBody.empty())
				continue;

			Content content;
			if (notifyId > 0) {
				ContentType contentType(ContentType::Multipart);
				contentType.setParameter("boundary=" + string(MultipartBoundary));
				content.setContentType(contentType);
			} else
				content.setContentType(ContentType::ConferenceInfo);

			content.setBody(notifyBody);
			content.addHeader("Content-Id", addr.asStringUriOnly());
			contents.push_back(content);

			// Add entry into the rlmi content of the notify body
		}
	}

	contents.push_front(rlmiContent);
	Content multipart = ContentManager::contentListToMultipart(contents);
	(void) multipart;
}

void LocalConferenceListEventHandler::notify () {

}

// -----------------------------------------------------------------------------

void LocalConferenceListEventHandler::addHandler (shared_ptr<LocalConferenceEventHandler> handler) {
	handlers.push_back(handler);
}

shared_ptr<LocalConferenceEventHandler> LocalConferenceListEventHandler::findHandler (const ChatRoomId &chatRoomId) const {
	for (const auto &handler : handlers) {
		if (handler->getChatRoomId() == chatRoomId)
			return handler;
	}

	return nullptr;
}

const list<shared_ptr<LocalConferenceEventHandler>> &LocalConferenceListEventHandler::getHandlers () const {
	return handlers;
}

LINPHONE_END_NAMESPACE

