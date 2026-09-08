/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DatabaseEnvFwd_h__
#define DatabaseEnvFwd_h__

#include "AsyncCallbackProcessorFwd.h"
#include <memory>

struct QueryResultFieldMetadata;
class Field;

class ResultSet;
using QueryResult = std::shared_ptr<ResultSet>;

class CharacterDatabaseConnection;
class HotfixDatabaseConnection;
class LoginDatabaseConnection;
class WorldDatabaseConnection;
//By leewheel 2026-09-06: 移植mod-playerbots，新增Playerbots数据库前向声明
class PlayerbotsDatabaseConnection;
//End By leewheel

class PreparedStatementBase;

template<typename T>
class PreparedStatement;

using CharacterDatabasePreparedStatement = PreparedStatement<CharacterDatabaseConnection>;
using HotfixDatabasePreparedStatement = PreparedStatement<HotfixDatabaseConnection>;
using LoginDatabasePreparedStatement = PreparedStatement<LoginDatabaseConnection>;
using WorldDatabasePreparedStatement = PreparedStatement<WorldDatabaseConnection>;
//By leewheel 2026-09-06: 移植mod-playerbots，新增Playerbots预处理语句别名
using PlayerbotsDatabasePreparedStatement = PreparedStatement<PlayerbotsDatabaseConnection>;
//End By leewheel

class PreparedResultSet;
using PreparedQueryResult = std::shared_ptr<PreparedResultSet>;

class QueryCallback;
bool InvokeAsyncCallbackIfReady(QueryCallback& callback);

using QueryCallbackProcessor = AsyncCallbackProcessor<QueryCallback>;

class TransactionBase;

template<typename T>
class Transaction;

class TransactionCallback;
bool InvokeAsyncCallbackIfReady(TransactionCallback& callback);

template<typename T>
using SQLTransaction = std::shared_ptr<Transaction<T>>;

using CharacterDatabaseTransaction = SQLTransaction<CharacterDatabaseConnection>;
using HotfixDatabaseTransaction = SQLTransaction<HotfixDatabaseConnection>;
using LoginDatabaseTransaction = SQLTransaction<LoginDatabaseConnection>;
using WorldDatabaseTransaction = SQLTransaction<WorldDatabaseConnection>;
//By leewheel 2026-09-06: 移植mod-playerbots，新增Playerbots事务别名
using PlayerbotsDatabaseTransaction = SQLTransaction<PlayerbotsDatabaseConnection>;
//End By leewheel

class SQLQueryHolderBase;

template<typename T>
class SQLQueryHolder;

using CharacterDatabaseQueryHolder = SQLQueryHolder<CharacterDatabaseConnection>;
using HotfixDatabaseQueryHolder = SQLQueryHolder<HotfixDatabaseConnection>;
using LoginDatabaseQueryHolder = SQLQueryHolder<LoginDatabaseConnection>;
using WorldDatabaseQueryHolder = SQLQueryHolder<WorldDatabaseConnection>;
//By leewheel 2026-09-06: 移植mod-playerbots，新增Playerbots查询持有者别名
using PlayerbotsDatabaseQueryHolder = SQLQueryHolder<PlayerbotsDatabaseConnection>;
//End By leewheel

class SQLQueryHolderCallback;
bool InvokeAsyncCallbackIfReady(SQLQueryHolderCallback& callback);

// mysql
struct MySQLHandle;
struct MySQLResult;
struct MySQLField;
struct MySQLBind;
struct MySQLStmt;

#endif // DatabaseEnvFwd_h__
